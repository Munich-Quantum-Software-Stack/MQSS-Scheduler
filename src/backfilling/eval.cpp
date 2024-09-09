/**
 * @file eval.cpp
 * @brief This file contains the implementation of the functions to evaluate and
 * extract information from a quantum circuit (no ML required).
 */
#include "eval.hpp"
#include <iostream>
#include <llvm/IR/InstrTypes.h>
#include <unordered_set>

using namespace llvm;

/*
 * @brief Extract features from a quantum circuit
 * @param TSM The quantum circuit to evaluate
 * @param gate_counts The counts of each gate in the circuit
 * @return A vector of the supermarq (plus 3 additional) features
 */
std::vector<double>
evaluate_supermarq_plus(const ThreadSafeModule &TSM,
                        std::map<std::string, int> &gate_counts) {
  std::string QIS_START = "__quantum__qis_";
  int num_gates = 0, num_two_qubit_gates = 0;
  std::unordered_map<std::string, int> qubit_counts, two_qubit_gate_counts,
      qubit_depth;
  std::unordered_map<std::string, std::unordered_set<std::string>>
      qubit_connections, dir_qubit_connections;

  if (!TSM) {
    std::cerr << "ThreadSafeModule is null" << std::endl;
    return std::vector<double>{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  }
  // Standard way to traverse the QIR circuit
  TSM.withModuleDo([&](Module &module) {
    LLVMContext &Context = module.getContext();
    StructType *qubitType = StructType::getTypeByName(Context, "Qubit");
    for (auto &function : module) {
      for (auto &block : function) {
        for (auto &instruction : block) {
          if (auto call_instr = dyn_cast<CallBase>(&instruction)) {
            if (auto f = call_instr->getCalledFunction()) {
              auto op_name = static_cast<std::string>(f->getName());

              bool is_quantum =
                  (op_name.size() >= QIS_START.size() &&
                   op_name.substr(0, QIS_START.size()) == QIS_START);

              if (is_quantum) {
                std::string ctrl_qubit = "";

                // Count each gate
                if (gate_counts.count(op_name) >= 0) {
                  gate_counts[op_name]++;
                  num_gates++;
                } else {
                  std::cerr << "Unknown gate: " << op_name << std::endl;
                }
                // Look for qubits affected by the gate
                for (Use &operand : call_instr->operands()) {
                  if (auto *val = dyn_cast<Value>(&operand)) {
                    if (val->getType() == PointerType::get(qubitType, 0)) {
                      std::string qubit;
                      llvm::raw_string_ostream stream(qubit);
                      operand.get()->printAsOperand(stream, true);
                      stream.flush();

                      // Count operations on each qubit
                      qubit_counts.emplace(qubit, 0).first->second++;

                      // Count 2-qubit gates
                      if (ctrl_qubit != "") {
                        std::string trgt_qubit = qubit;
                        // Count connections between qubits
                        // Undirected graph
                        qubit_connections[trgt_qubit].insert(ctrl_qubit);
                        qubit_connections[ctrl_qubit].insert(trgt_qubit);
                        // Directed graph
                        dir_qubit_connections[ctrl_qubit].insert(trgt_qubit);

                        // Initialize qubit depth if not found
                        qubit_depth.emplace(trgt_qubit, 0);

                        // Ctrl and Trgt qubit must have same depth
                        int ctrl_qubit_depth = qubit_depth[ctrl_qubit];
                        int trgt_qubit_depth = qubit_depth[trgt_qubit];

                        if (ctrl_qubit_depth > trgt_qubit_depth) {
                          // Ctrl qubit was already incremented before
                          qubit_depth[trgt_qubit] = ctrl_qubit_depth;
                        } else if (trgt_qubit_depth >= ctrl_qubit_depth) {
                          // Account for current gate
                          qubit_depth[trgt_qubit]++;
                          // Both qubits have the same depth now
                          qubit_depth[ctrl_qubit] = qubit_depth[trgt_qubit];
                        }

                        two_qubit_gate_counts.emplace(ctrl_qubit, 0)
                            .first->second++;
                        two_qubit_gate_counts.emplace(trgt_qubit, 0)
                            .first->second++;
                        num_two_qubit_gates++;
                      } else {
                        // Account for current gate
                        qubit_depth.emplace(qubit, 0).first->second++;
                        // In case its 2-qubit gate
                        ctrl_qubit = qubit;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  });

  // Calculate the circuit depth
  int depth = 0;
  std::string max_depth_qubit;
  for (const auto &pair : qubit_depth) {
    if (pair.second > depth) {
      depth = pair.second;
      max_depth_qubit = pair.first;
    }
  }

  // Count non-idle qubits (see activity matrix)
  int activity_count = 0;
  for (const auto &pair : qubit_counts) {
    activity_count += pair.second;
  }

  // Number of active qubits the circuit
  int num_qubits = qubit_counts.size();

  // Calculate the (directed_)programm_communication
  int degree_sum = 0, degree_sum_di = 0;
  for (const auto &pair : qubit_connections) {
    degree_sum += pair.second.size();
  }
  for (const auto &pair : dir_qubit_connections) {
    degree_sum_di += pair.second.size();
  }
  double program_communication =
      num_qubits > 1 ? (double)degree_sum / (num_qubits * (num_qubits - 1)) : 0;
  double directed_program_communication =
      num_qubits > 1 ? (double)degree_sum_di / (num_qubits * (num_qubits - 1))
                     : 0;

  // Calculate critical depth
  double critical_depth = num_two_qubit_gates > 0
                              ? (double)two_qubit_gate_counts[max_depth_qubit] /
                                    (double)num_two_qubit_gates
                              : 0.0;

  // Calculate entanglement ratio
  double entanglement_ratio =
      num_gates > 0 ? (double)num_two_qubit_gates / (double)num_gates : 0.0;

  // Calculate average number of gates per layer
  double single_qubit_gate_ratio =
      (num_qubits > 0 && depth > 0)
          ? (double)(num_gates - num_two_qubit_gates) /
                (double)(depth * num_qubits)
          : 0.0;
  double two_qubit_gate_ratio =
      (num_qubits > 0 && depth > 0)
          ? (double)num_two_qubit_gates /
                (double)(depth * (int)(num_qubits / 2))
          : 0.0;

  // Calculate parallelism
  double parallelism = (num_qubits > 1 && depth > 0)
                           ? (((double)num_gates / (double)depth) - 1) *
                                 (1 / (double)(num_qubits - 1))
                           : 0.0;

  // Calculate liveness
  double liveness = (num_qubits > 1 && depth > 0)
                        ? (double)activity_count / (double)(depth * num_qubits)
                        : 0.0;

  return std::vector<double>{static_cast<double>(num_qubits),
                             static_cast<double>(depth),
                             // original supermarq features
                             program_communication, critical_depth,
                             entanglement_ratio, parallelism, liveness,
                             // plus additional features
                             directed_program_communication,
                             single_qubit_gate_ratio, two_qubit_gate_ratio};
}

/*
 * @brief Calculate the duration of a quantum circuit based on the gate times
 * along its critical path (longest in circuit graph).
 * @param TSM The quantum circuit to evaluate
 * @param single_qubit_gate_time The time taken for a single qubit gate
 * @param multi_qubit_gate_time The time taken for a multi qubit gate
 * @param measurement_time The time taken for a measurement
 * @return The duration of the circuit
 */
double calculate_circuit_duration(ThreadSafeModule &TSM,
                                  double single_qubit_gate_time,
                                  double multi_qubit_gate_time,
                                  double measurement_time) {
  std::string QIS_START = "__quantum__qis_";
  double circuit_duration = 0.0;
  // To sum the operation times on each qubit
  std::unordered_map<std::string, double> qubit_times;

  if (!TSM) {
    std::cerr << "ThreadSafeModule is null" << std::endl;
    return 0.0;
  }
  // Standard way to traverse the QIR circuit
  TSM.withModuleDo([&](Module &module) {
    LLVMContext &Context = module.getContext();
    StructType *qubitType = StructType::getTypeByName(Context, "Qubit");
    for (auto &function : module) {
      for (auto &block : function) {
        for (auto &instruction : block) {
          if (auto call_instr = dyn_cast<CallBase>(&instruction)) {
            if (auto f = call_instr->getCalledFunction()) {
              auto op_name = static_cast<std::string>(f->getName());

              bool is_quantum =
                  (op_name.size() >= QIS_START.size() &&
                   op_name.substr(0, QIS_START.size()) == QIS_START);

              if (is_quantum) {
                double gate_time = 0.0;
                if (op_name == "__quantum__qis__mz__body") {
                  gate_time = measurement_time;
                } else if (call_instr->getNumOperands() > 1) {
                  gate_time = multi_qubit_gate_time;
                } else {
                  gate_time = single_qubit_gate_time;
                }

                // Look for qubits affected by the gate
                for (Use &operand : call_instr->operands()) {
                  if (auto *val = dyn_cast<Value>(&operand)) {
                    if (val->getType() == PointerType::get(qubitType, 0)) {
                      std::string qubit;
                      llvm::raw_string_ostream stream(qubit);
                      operand.get()->printAsOperand(stream, true);
                      stream.flush();

                      // Add gate time to qubits time
                      qubit_times[qubit] += gate_time;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  });

  // Return the maximum qubit time
  for (const auto &pair : qubit_times) {
    if (pair.second > circuit_duration) {
      circuit_duration = pair.second;
    }
  }
  return circuit_duration;
}

/*
 * @brief Extract the number of active qubits from a quantum circuit
 * @param TSM The quantum circuit to evaluate
 * @return The number of active qubits
 */
int evaluate_num_qubits(const ThreadSafeModule &TSM) {
  std::string QIS_START = "__quantum__qis_";
  std::unordered_map<std::string, int> qubit_counts;

  if (!TSM) {
    std::cerr << "ThreadSafeModule is null" << std::endl;
    return 0;
  }
  // Standard way to traverse the QIR circuit
  TSM.withModuleDo([&](Module &module) {
    LLVMContext &Context = module.getContext();
    StructType *qubitType = StructType::getTypeByName(Context, "Qubit");
    for (auto &function : module) {
      for (auto &block : function) {
        for (auto &instruction : block) {
          if (auto call_instr = dyn_cast<CallBase>(&instruction)) {
            if (auto f = call_instr->getCalledFunction()) {
              auto op_name = static_cast<std::string>(f->getName());

              bool is_quantum =
                  (op_name.size() >= QIS_START.size() &&
                   op_name.substr(0, QIS_START.size()) == QIS_START);

              if (is_quantum) {
                // Look for qubits affected by the gate
                for (Use &operand : call_instr->operands()) {
                  if (auto *val = dyn_cast<Value>(&operand)) {
                    if (val->getType() == PointerType::get(qubitType, 0)) {
                      std::string qubit;
                      llvm::raw_string_ostream stream(qubit);
                      operand.get()->printAsOperand(stream, true);
                      stream.flush();
                      // Count operations on each
                      // qubit
                      qubit_counts[qubit]++;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  });

  // Number of active qubits in the circuit
  int num_qubits = qubit_counts.size();

  return num_qubits;
}
