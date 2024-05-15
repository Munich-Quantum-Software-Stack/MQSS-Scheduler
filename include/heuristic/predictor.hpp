#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "qdmi.h"
#include <array>
#include <cstdio>
#include <map>
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <vector>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Module.h>

using llvm::orc::ThreadSafeModule;

#define MODEL "/home/ubuntu/scheduler/inputs/model.onnx"
std::map<std::string, float> predict(const ThreadSafeModule &TSM,
                                     const std::vector<std::string> models);

#endif // PREDICTOR_H
