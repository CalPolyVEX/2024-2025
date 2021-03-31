#include <torch/script.h> // One-stop header.
#include <torch/torch.h>

//#include "ros/ros.h"

#include <iostream>
#include <memory>
#include <chrono>
using namespace std::chrono;

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: example-app <path-to-exported-script-module>\n";
    return -1;
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
