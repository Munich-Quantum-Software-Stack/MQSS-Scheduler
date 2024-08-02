#ifndef EVAL_HPP
#define EVAL_HPP

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <map>
#include <vector>

using llvm::orc::ThreadSafeModule;

std::vector<double>
evaluate_supermarq_plus(const ThreadSafeModule &TSM,
                        std::map<std::string, int> &gate_counts);

double calculate_circuit_duration(ThreadSafeModule &TSM,
                                  double single_qubit_gate_time,
                                  double multi_qubit_gate_time,
                                  double measurement_time);

int evaluate_num_qubits(const ThreadSafeModule &TSM);

#endif
