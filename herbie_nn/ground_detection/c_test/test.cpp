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
struct TRTDestroy
{
    template< class T >
    void operator()(T* obj) const
    {
        if (obj)
        {
            obj->destroy();
        }
    }
};

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
    config->setMaxWorkspaceSize(1 << 20);

    //set dimensions
    nvinfer1::IOptimizationProfile* profile = builder->createOptimizationProfile();
    profile->setDimensions("input", nvinfer1::OptProfileSelector::kMIN, nvinfer1::Dims4(1,3,360,640));
    profile->setDimensions("input", nvinfer1::OptProfileSelector::kOPT, nvinfer1::Dims4(1,3,360,640));
    profile->setDimensions("input", nvinfer1::OptProfileSelector::kMAX, nvinfer1::Dims4(1,3,360,640));
    
    config->addOptimizationProfile(profile);

    nvinfer1::ICudaEngine* engine = builder->buildEngineWithConfig(*network, *config);

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

/////////////////////////////////////////////////
int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: example-app <path-to-exported-script-module>\n";
    return -1;
  }
  
  //test serialization using TensorRT 
  //parseOnnxModel();

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
