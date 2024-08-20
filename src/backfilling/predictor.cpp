/**
 * @file predictor.cpp
 * @brief Here the ML related code is implemented. The predict function does
 * inference on the given quantum circuit using the trained models passed to it.
 */
#include "predictor.hpp"
#include "eval.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

/*
 * @brief Predict some figure of merit based on a pretrained ONNX model for each
 * model
 * @param TSM The quantum circuit to evaluate
 * @param models The trained models to predict some figure of merit
 * @return A map from each model to the predicted figure of merit
 */
std::unordered_map<std::string, float>
predict(const ThreadSafeModule &TSM, const std::vector<std::string> models) {
  // Prepare circuit feature vector for model input (order is important)
  std::map<std::string, int> gateCounts = {{"__quantum__qis__U3__body", 0},
                                           {"u2", 0},
                                           {"u1", 0},
                                           {"__quantum__qis__cnot__body", 0},
                                           {"id", 0},
                                           {"u0", 0},
                                           {"u", 0},
                                           {"p", 0},
                                           {"__quantum__qis__x__body", 0},
                                           {"__quantum__qis__y__body", 0},
                                           {"__quantum__qis__z__body", 0},
                                           {"__quantum__qis__h__body", 0},
                                           {"__quantum__qis__s__body", 0},
                                           {"sdg", 0},
                                           {"__quantum__qis__t__body", 0},
                                           {"tdg", 0},
                                           {"__quantum__qis__rx__body", 0},
                                           {"__quantum__qis__ry__body", 0},
                                           {"__quantum__qis__rz__body", 0},
                                           {"sx", 0},
                                           {"sxdg", 0},
                                           {"__quantum__qis__cz__body", 0},
                                           {"__quantum__qis__cy__body", 0},
                                           {"swap", 0},
                                           {"ch", 0},
                                           {"ccx", 0},
                                           {"cswap", 0},
                                           {"crx", 0},
                                           {"cry", 0},
                                           {"crz", 0},
                                           {"cu1", 0},
                                           {"cp", 0},
                                           {"cu3", 0},
                                           {"csx", 0},
                                           {"cu", 0},
                                           {"rxx", 0},
                                           {"rzz", 0},
                                           {"rccx", 0},
                                           {"rc3x", 0},
                                           {"c3x", 0},
                                           {"c3sqrtx", 0},
                                           {"c4x", 0},
                                           {"__quantum__qis__mz__body", 0}};

  // Create a memory information object
  Ort::MemoryInfo memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  // Generate supermarq+ features
  std::vector<double> supermarqPlus = evaluate_supermarq_plus(TSM, gateCounts);

  // Prepare input tensor
  std::array<float, 52> inputData;
  int i = 0;
  // Fill inputData with gateCounts values
  for (const auto &pair : gateCounts) {
    inputData[i++] = (float)(pair.second);
  }
  // Fill the rest of inputData with supermarqPlus values
  for (const double &value : supermarqPlus) {
    inputData[i++] = (float)(value);
  }
  std::vector<int64_t> inputShape = {1, 52};
  Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
      memoryInfo, inputData.data(), inputData.size(), inputShape.data(),
      inputShape.size());

  // Set the input
  std::vector<const char *> inputNodeNames = {"float_input"};
  std::vector<Ort::Value> inputTensors;
  inputTensors.push_back(std::move(inputTensor));

  // Initialize session options
  Ort::SessionOptions sessionOptions;
  sessionOptions.SetIntraOpNumThreads(1);

  // Initialize the environment
  Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Predictor");

  // Create a map to store the results
  std::unordered_map<std::string, float> results;

  // Get the current file path
  std::filesystem::path currentFilePath = __FILE__;
  std::filesystem::path currentDir = currentFilePath.parent_path();
  // Navigate up to the 'scheduler' directory, then to the 'models' directory
  std::filesystem::path modelsDir =
      currentDir.parent_path().parent_path() / "models";

  // Loop over all models
  for (const std::string &model : models) {
    // Declare the session pointer
    Ort::Session *session = nullptr;

    // Import the model for desired figure of merit
    std::filesystem::path modelPath = modelsDir / model;

    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    try {
      std::vector<char> buffer(size);
      if (file.read(buffer.data(), size)) {
        // Initialize the session
        session =
            new Ort::Session(env, buffer.data(), buffer.size(), sessionOptions);
      } else {
        throw std::runtime_error("Failed to read model file: " +
                                 modelPath.string());
      }
    } catch (const std::exception &ex) {
      std::cerr << "Error during model import: " << ex.what() << std::endl;
    }

    // Prepare output tensor
    std::vector<const char *> outputNodeNames = {"variable"};
    std::array<float, 1> outputData;
    std::vector<int64_t> outputShape = {1, 1};
    Ort::Value outputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, outputData.data(), outputData.size(), outputShape.data(),
        outputShape.size());

    // Add output_tensor to output_tensors
    std::vector<Ort::Value> outputTensors;
    outputTensors.push_back(std::move(outputTensor));

    // Run the model
    try {
      session->Run(Ort::RunOptions{nullptr}, inputNodeNames.data(),
                   inputTensors.data(), inputTensors.size(),
                   outputNodeNames.data(), outputTensors.data(),
                   outputTensors.size());
    } catch (const std::exception &ex) {
      std::cerr << "Error during model inference: " << ex.what() << std::endl;
    }

    // Add the model string and model score to the map
    float *floatarr = outputTensors[0].GetTensorMutableData<float>();
    results[model] = floatarr[0];

    // Delete the session after use
    delete session;
  }

  // Return the map of model strings and model scores
  return results;
}
