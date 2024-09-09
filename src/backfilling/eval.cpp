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

// DFS helper function to find the longest/critical path in a circuit graph
// Returns its depth and the number of 2-qubit gates along the path
std::pair<int, int>
dfs(const std::string &node,
    const std::unordered_map<std::string, std::vector<std::string>>
        &adjacencyList,
    std::unordered_map<std::string, std::pair<int, int>> &visited) {

  if (node.empty()) {
    // Input node (not a gate)
    return {0, 0};
  }
  if (visited.find(node) != visited.end()) {
    return visited[node];
  }

  int maxDepth = -1; // Initialize to -1 to exclude the input node (not a gate)
  int numTwoQubitGates = 0;

  auto it = adjacencyList.find(node);
  if (it != adjacencyList.end()) {
    for (const auto &neighbor : it->second) {
      auto result = dfs(neighbor, adjacencyList, visited);
      if (result.first > maxDepth) {
        maxDepth = result.first;
        numTwoQubitGates = result.second;
      }
    }
  } else {
    throw std::out_of_range("Node not found in adjacency list");
  }

  // Check if the current node is a 2-qubit gate
  if (it->second.size() == 2) {
    numTwoQubitGates++;
  }

  visited[node] = {maxDepth + 1, numTwoQubitGates};
  return visited[node];
}

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
  std::unordered_map<std::string, std::vector<std::string>> qubit_gate_counts,
      adjacency_list;
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

                // Create unique id for each node in cirucit graph
                std::string op_id = op_name + "_" + std::to_string(num_gates);

                auto it = gate_counts.find(op_name);
                if (it == gate_counts.end()) {
                  std::cerr << "Unknown gate: " << op_name << std::endl;
                } else {
                  // Increase total gate count
                  it->second++;
                  num_gates++;
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

                      // Get last operation on qubit
                      auto last_op = qubit_gate_counts[qubit].empty()
                                         ? qubit
                                         : qubit_gate_counts[qubit].back();

                      // Add current operation to qubit's operations
                      qubit_gate_counts
                          .emplace(qubit, std::vector<std::string>())
                          .first->second.push_back(op_id);

                      // Add input node that does not have any predecessors
                      if (last_op == qubit) {
                        adjacency_list.emplace(qubit,
                                               std::vector<std::string>{});
                      }
                      // Add node to adjacency list
                      auto res = adjacency_list.emplace(
                          op_id, std::vector<std::string>{last_op});
                      if (!res.second) {
                        // If the key already exists, add last_op
                        res.first->second.push_back(last_op);
                      }

                      if (ctrl_qubit == "") {
                        // In case its 2-qubit gate
                        ctrl_qubit = qubit;
                      } else {
                        // Count 2-qubit gates
                        std::string trgt_qubit = qubit;
                        // Count connections between qubits in:
                        // UNdirected graph
                        qubit_connections[trgt_qubit].insert(ctrl_qubit);
                        qubit_connections[ctrl_qubit].insert(trgt_qubit);
                        // DIrected graph
                        dir_qubit_connections[ctrl_qubit].insert(trgt_qubit);

                        num_two_qubit_gates++;
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

  // Calculate the depth based on the longest path in the circuit graph
  std::unordered_map<std::string, std::pair<int, int>> visited;
  int depth = 0;
  int numTwoQubitGatesOnCriticalPath = 0;

  for (const auto &pair : adjacency_list) {
    auto result = dfs(pair.first, adjacency_list, visited);
    if (result.first > depth) {
      depth = result.first;
      numTwoQubitGatesOnCriticalPath = result.second;
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
  double critical_depth =
      num_two_qubit_gates > 0
          ? (double)numTwoQubitGatesOnCriticalPath / (double)num_two_qubit_gates
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
