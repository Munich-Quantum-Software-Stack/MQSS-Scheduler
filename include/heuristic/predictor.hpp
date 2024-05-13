#ifndef PREDICTOR_H
#define PREDICTOR_H

#include "qdmi.h"
#include <array>
#include <cstdio>
#include <map>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <vector>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Module.h>

using llvm::orc::ThreadSafeModule;

#define MODEL "/home/ubuntu/scheduler/inputs/model.onnx"
float predict(ThreadSafeModule &TSM, QDMI_Device device);

#endif // PREDICTOR_H
