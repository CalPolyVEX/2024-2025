#include "camera.h"

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
       std::cout << "Using DLA Core 0" << std::endl;
    } else {
       std::cout << "Using DLA Core 1" << std::endl;
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
void CameraReader::initInference(char* s) {
   struct nn_context* nn = &nn1;
   if (network_counter == 0) {
      network_counter++;
      nn = &nn1;
   } else {
      nn = &nn2;
   }

   std::ifstream file(s, std::ios::binary);
   size_t engine_size{0};
   std::vector<char> trtModelStream_;

   /////////////////////////////////////
   //read in the engine data
   if (file.good())
   {
      file.seekg(0, file.end);
      engine_size = file.tellg();
      file.seekg(0, file.beg);
      trtModelStream_.resize(engine_size);
      std::cout << "size" << trtModelStream_.size() << std::endl;
      file.read(trtModelStream_.data(), engine_size);
      file.close();
   }

   ////////////////////////////////////////////////
   //create the inference runtime
   nn->runtime = nvinfer1::createInferRuntime(gLogger);
   assert(nn->runtime != nullptr);
   nn->engine = nn->runtime->deserializeCudaEngine(trtModelStream_.data(), engine_size, nullptr);

   ////////////////////////////////////////////////////////
   //perform inference
   nn->context = nn->engine->createExecutionContext();

   //allocate space
   //void** mInputCPU= (void**)malloc(2*sizeof(void*));
   nn->mInputCPU = (void **)malloc(2 * sizeof(void *));
   cudaHostAlloc((void **)&nn->mInputCPU[0], 3 * 360 * 640 * sizeof(float), cudaHostAllocDefault);
   cudaHostAlloc((void **)&nn->mInputCPU[1], 1 * 80 * sizeof(float), cudaHostAllocDefault);

   nn->inputIndex = nn->engine->getBindingIndex("input");
   nn->outputIndex = nn->engine->getBindingIndex("output");

   std::cout << nn->engine->getNbLayers() << std::endl;
   std::cout << nn->inputIndex << std::endl;
   std::cout << nn->outputIndex << std::endl;

   // create GPU buffers and a stream
   gpuErrchk(cudaMalloc(&nn->buffers[nn->inputIndex], 1 * 3 * 360 * 640 * sizeof(float)));
   gpuErrchk(cudaMalloc(&nn->buffers[nn->outputIndex], 1 * 80 * sizeof(float)));

   gpuErrchk(cudaStreamCreate(&nn->stream));
}

///////////////////////////////////
//run inference
void CameraReader::inference(struct nn_context* nn) {
   auto start = high_resolution_clock::now();
   // DMA the input to the GPU,  execute the batch asynchronously, and DMA it back:
   gpuErrchk( cudaMemcpyAsync(nn->buffers[nn->inputIndex], nn->mInputCPU[0], 1*3*360*640 * sizeof(float), cudaMemcpyHostToDevice, nn->stream) );

   nn1.context->executeV2(nn->buffers);

   gpuErrchk( cudaMemcpyAsync(nn->mInputCPU[1], nn->buffers[nn->outputIndex], 1*80*sizeof(float), cudaMemcpyDeviceToHost, nn->stream) );

   cudaStreamSynchronize(nn->stream);

   auto end = high_resolution_clock::now();
   auto duration = duration_cast<microseconds>(end - start) / 1000.0;
   std::cout << duration.count() << std::endl;
}

///////////////////////////////////
//clean up inference
void CameraReader::endInference() {
  // release the stream and the buffers
  cudaStreamDestroy(nn1.stream);
  gpuErrchk( cudaFree(nn1.buffers[nn1.inputIndex]) );
  gpuErrchk( cudaFree(nn1.buffers[nn1.outputIndex]) );
  
  // destroy the engine
  nn1.context->destroy();
  nn1.engine->destroy();
}
