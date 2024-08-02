#ifndef PREDICTOR_H
#define PREDICTOR_H

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <unordered_map>
#include <vector>

using llvm::orc::ThreadSafeModule;

std::unordered_map<std::string, float>
predict(const ThreadSafeModule &TSM, const std::vector<std::string> models);

#endif
