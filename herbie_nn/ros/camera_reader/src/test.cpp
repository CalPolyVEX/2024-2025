#include <torch/script.h> // One-stop header.
#include <torch/torch.h>

//#include "ros/ros.h"

#include <iostream>
#include <memory>
#include <chrono>
using namespace std::chrono;

//Tensorrt includes
//#include "parserOnnxConfig.h"
#include <cuda_runtime_api.h>
#include "NvOnnxParser.h"
#include "NvInfer.h"

void writeNetworkTensorNames(nvinfer1::INetworkDefinition *network);
bool setDynamicRange(nvinfer1::INetworkDefinition *network);
std::unordered_map<std::string, float>
  mPerTensorDynamicRangeMap; //!< Mapping from tensor name to max absolute dynamic range values

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

// destroy TensorRT objects if something goes wrong
/* struct TRTDestroy */
/* { */
/*     template< class T > */
/*     void operator()(T* obj) const */
/*     { */
/*         if (obj) */
/*         { */
/*             obj->destroy(); */
/*         } */
/*     } */
/* }; */

//////////////////////////////
//parse and serialize the tensorrt engine
void parseOnnxModel() 
{
    int maxBatchSize = 1;

    nvinfer1::IBuilder* builder = nvinfer1::createInferBuilder(gLogger);
    const auto explicitBatch = 1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    nvinfer1::INetworkDefinition* network = builder->createNetworkV2(explicitBatch);

    nvonnxparser::IParser* parser = nvonnxparser::createParser(*network, gLogger);

    parser->parseFromFile("./test_model.onnx", 1);
    for (int i = 0; i < parser->getNbErrors(); ++i)
    {
        std::cout << parser->getError(i)->desc() << std::endl;
    }

    builder->setMaxBatchSize(maxBatchSize);
    nvinfer1::IBuilderConfig* config = builder->createBuilderConfig();
    config->setMaxWorkspaceSize(1 << 30);

    //set dimensions
//    nvinfer1::IOptimizationProfile* profile = builder->createOptimizationProfile();
    /* profile->setDimensions("input", nvinfer1::OptProfileSelector::kMIN, nvinfer1::Dims4(1,3,360,640)); */
    /* profile->setDimensions("input", nvinfer1::OptProfileSelector::kOPT, nvinfer1::Dims4(1,3,360,640)); */
    /* profile->setDimensions("input", nvinfer1::OptProfileSelector::kMAX, nvinfer1::Dims4(1,3,360,640)); */
    
    //config->addOptimizationProfile(profile);

    //indicate fp16 is acceptable
    config->setFlag(nvinfer1::BuilderFlag::kFP16);
    /* config->setFlag(nvinfer1::BuilderFlag::kINT8); */
    config->setFlag(nvinfer1::BuilderFlag::kSTRICT_TYPES);

    //test DLA code
    /* config->setFlag(nvinfer1::BuilderFlag::kGPU_FALLBACK); */
    /* config->setDefaultDeviceType(nvinfer1::DeviceType::kDLA); */
    /* config->setDLACore(0); */

    setDynamicRange(network);

    nvinfer1::ICudaEngine* engine = builder->buildEngineWithConfig(*network, *config);

    //writeNetworkTensorNames(network); //write out the tensor names

    parser->destroy();
    network->destroy();
    config->destroy();
    builder->destroy();

    //serialize the model to a file
    nvinfer1::IHostMemory *serializedModel = engine->serialize();

    std::ofstream engine_file("test_model.engine");

    engine_file.write((const char*)serializedModel->data(),serializedModel->size());

    serializedModel->destroy();
}

//////////////////////////////
//deserialize engine
void load_engine() {
  std::vector<char> trtModelStream_;
  size_t size{ 0 };

  std::ifstream file("test_model.engine", std::ios::binary);
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
  cudaHostAlloc((void**)&mInputCPU[1],  3*360*640*sizeof(float), cudaHostAllocDefault);
  //float prob[80]; //output data
  
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
    //context->enqueue(1, buffers, stream, nullptr);
    context->executeV2(buffers);
    /* context->enqueueV2(buffers, stream, nullptr); */
    //gpuErrchk( cudaMemcpyAsync(prob, buffers[outputIndex], 1*80*sizeof(float), cudaMemcpyDeviceToHost, stream) );
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

/////////////////////////////////////////////////
int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: example-app <path-to-exported-script-module>\n";
    return -1;
  }
  
  //test serialization using TensorRT 
  if (strcmp(argv[1],"build") == 0) {
    std::cout << "Building engine." << std::endl;
    parseOnnxModel();
    return 0;
  }
  else if (strcmp(argv[1],"load") == 0) {
    std::cout << "Running engine." << std::endl;
    load_engine();
    return 0;
  }

  torch::jit::script::Module module;
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    module = torch::jit::load(argv[1], torch::kCUDA);
  }
  catch (const c10::Error& e) {
    std::cerr << "error loading the model\n";
    return -1;
  }

  std::cout << "model loaded successfully.\n";

  for(int i=0; i<1000000; i++) {
    auto input = torch::randn({3, 360, 640});
    input = input.unsqueeze(0);
    auto start = high_resolution_clock::now();

    // Create a vector of inputs.
    torch::Tensor gpu_tensor = input.to(torch::kCUDA);
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(gpu_tensor);
    
    // Execute the model and turn its output into a tensor.
    at::Tensor output = module.forward(inputs).toTensor();

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start) / 1000.0;
    std::cout << output.slice(1,0,8) << std::endl;
    std::cout << duration.count() << std::endl;
  }
}
