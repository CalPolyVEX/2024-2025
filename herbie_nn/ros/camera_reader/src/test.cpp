#include "camera.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <cassert>
#include <cstring>

//Tensorrt includes
#include <cuda_runtime_api.h>
#include "NvOnnxParser.h"
#include "NvInfer.h"

using namespace std::chrono;

extern void writeNetworkTensorNames(nvinfer1::INetworkDefinition *network);
extern bool setDynamicRange(nvinfer1::INetworkDefinition *network);

//Mapping from tensor name to max absolute dynamic range values
std::unordered_map<std::string, float> mPerTensorDynamicRangeMap; 

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess)
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

class Logger : public nvinfer1::ILogger
{
public:
    void log(Severity severity, const char* msg) override {
        // remove this 'if' if you need more logged info
        if ((severity == Severity::kERROR) || (severity == Severity::kINTERNAL_ERROR)) {
            std::cout << msg << "n";
        }
    }
} gLogger;

//////////////////////////////////////
//build the engine from the ONNX model
void CameraReader::buildEngine(char* s, int dla) 
{
    //do some filename conversions first
    boost::filesystem::path p((char*) &s[0]); //convert the path
    char onnx_filename_path[200];
    char engine_filename[200];
    strncpy(onnx_filename_path, p.c_str(), 200);
    strncpy(engine_filename, boost::filesystem::change_extension(onnx_filename_path, ".engine").c_str(), 200);

    std::cout << "input ONNX file: " << onnx_filename_path << std::endl;
    std::cout << "output engine file: " << engine_filename << std::endl;

    if (dla == 0) {
       std::cout << "Using GPU" << std::endl;
    } else if (dla == 1) {
       std::cout << "Using DLA 0" << std::endl;
    } else {
       std::cout << "Using DLA 1" << std::endl;
    }

    //start creating the network and parser
    nvinfer1::IBuilder* builder = nvinfer1::createInferBuilder(gLogger);
    const auto explicitBatch = 1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    nvinfer1::INetworkDefinition* network = builder->createNetworkV2(explicitBatch);

    nvonnxparser::IParser* parser = nvonnxparser::createParser(*network, gLogger);
    
    //parse the ONNX file
    parser->parseFromFile(onnx_filename_path, 1); 
    for (int i = 0; i < parser->getNbErrors(); ++i)
    {
        std::cout << parser->getError(i)->desc() << std::endl;
    }

    builder->setMaxBatchSize(1); //set the batch size to 1
    nvinfer1::IBuilderConfig* config = builder->createBuilderConfig();
    config->setMaxWorkspaceSize(1 << 30);

    //test DLA code
    if (dla == 0) { //use the GPU
       //indicate fp16 is acceptable
       config->setFlag(nvinfer1::BuilderFlag::kFP16);
    }
    else if (dla == 1) { //use DLA core 0
       config->setFlag(nvinfer1::BuilderFlag::kFP16);
       config->setFlag(nvinfer1::BuilderFlag::kSTRICT_TYPES);
       config->setFlag(nvinfer1::BuilderFlag::kGPU_FALLBACK);
       config->setDefaultDeviceType(nvinfer1::DeviceType::kDLA);
       config->setDLACore(0);
    } else if (dla == 2) { //use DLA core 1
       config->setFlag(nvinfer1::BuilderFlag::kFP16);
       config->setFlag(nvinfer1::BuilderFlag::kSTRICT_TYPES);
       config->setFlag(nvinfer1::BuilderFlag::kGPU_FALLBACK);
       config->setDefaultDeviceType(nvinfer1::DeviceType::kDLA);
       config->setDLACore(1);
    }

    setDynamicRange(network);

    nvinfer1::ICudaEngine* engine = builder->buildEngineWithConfig(*network, *config);

    //writeNetworkTensorNames(network); //write out the tensor names

    parser->destroy();
    network->destroy();
    config->destroy();
    builder->destroy();

    //serialize the model to a file
    nvinfer1::IHostMemory *serializedModel = engine->serialize();
    std::ofstream engine_file(engine_filename);
    engine_file.write((const char*)serializedModel->data(),serializedModel->size());
    std::cout << "Engine serialized to: " << engine_filename << std::endl;

    serializedModel->destroy();
}

///////////////////////////////////
//load a serialized engine
void CameraReader::loadEngine(char* s) {
  std::vector<char> trtModelStream_;
  size_t size{ 0 };

  std::ifstream file(s, std::ios::binary);

  if (file.good())
  {
    file.seekg(0, file.end);
    size = file.tellg();
    file.seekg(0, file.beg);
    trtModelStream_.resize(size);
    std::cout << "size" << trtModelStream_.size() << std::endl;
    file.read(trtModelStream_.data(), size);
    file.close();
  }
  std::cout << "size" << size << std::endl;
  nvinfer1::IRuntime* runtime = nvinfer1::createInferRuntime(gLogger);
  assert(runtime != nullptr);
  nvinfer1::ICudaEngine* engine = runtime->deserializeCudaEngine(trtModelStream_.data(), size, nullptr);

  ////////////////////////////////////////////////////////
  //perform inference
  nvinfer1::IExecutionContext *context = engine->createExecutionContext();

  //allocate space
  void** mInputCPU= (void**)malloc(2*sizeof(void*));;
  cudaHostAlloc((void**)&mInputCPU[0],  3*360*640*sizeof(float), cudaHostAllocDefault);
  cudaHostAlloc((void**)&mInputCPU[1],  1*80*sizeof(float), cudaHostAllocDefault);
  
  // In order to bind the buffers, we need to know the names of the input and output tensors.
  // note that indices are guaranteed to be less than IEngine::getNbBindings()
  int inputIndex = engine->getBindingIndex("input");
  int outputIndex = engine->getBindingIndex("output");

  std::cout << engine->getNbLayers() << std::endl;
  std::cout << inputIndex << std::endl;
  std::cout << outputIndex << std::endl;

  void* buffers[2];

  // create GPU buffers and a stream
  gpuErrchk( cudaMalloc(&buffers[inputIndex], 1 * 3 * 360 * 640 * sizeof(float)) );
  gpuErrchk( cudaMalloc(&buffers[outputIndex], 1 * 80 * sizeof(float)) );

  cudaStream_t stream;
  gpuErrchk( cudaStreamCreate(&stream) );

  for (int i=0; i<10000; i++) {
    auto start = high_resolution_clock::now();
    // DMA the input to the GPU,  execute the batch asynchronously, and DMA it back:
    gpuErrchk( cudaMemcpyAsync(buffers[inputIndex], mInputCPU[0], 1*3*360*640 * sizeof(float), cudaMemcpyHostToDevice, stream) );

    context->executeV2(buffers);

    gpuErrchk( cudaMemcpyAsync(mInputCPU[1], buffers[outputIndex], 1*80*sizeof(float), cudaMemcpyDeviceToHost, stream) );

    cudaStreamSynchronize(stream);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start) / 1000.0;
    std::cout << duration.count() << std::endl;
  }

  // release the stream and the buffers
  cudaStreamDestroy(stream);
  gpuErrchk( cudaFree(buffers[inputIndex]) );
  gpuErrchk( cudaFree(buffers[outputIndex]) );
  
  // destroy the engine
  context->destroy();
  engine->destroy();
}
