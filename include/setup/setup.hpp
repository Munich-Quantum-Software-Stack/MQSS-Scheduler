#ifndef SETUP_HPP
#define SETUP_HPP

#include "queue.hpp"
#include <memory>
#include <qdmi/client.h>
#include <quantum_task.hpp>
#include <submitter.hpp>
#include <vector>

// Macro for error checking
#define CHECK_ERR(a, b)                                                        \
  {                                                                            \
    if (a != QDMI_SUCCESS) {                                                   \
      std::cerr << std::endl << "[Error]: " << a << " at " << b;               \
    }                                                                          \
  }

// Create task with random preferred devices and random circuit
std::shared_ptr<QuantumTask>
createRandomTask(int taskID, const std::vector<QDMI_Device> &preferredDevices,
                 std::shared_ptr<QuantumTask> parentTask = nullptr,
                 int priority = -1, double duration = -1, int shots = -1);

// Prepare ScheduerQueues and associated Submitters
std::vector<std::shared_ptr<SchedulerQueue>> prepareQueues();

// Wait for all tasks to be executed (removed from the queues)
int allFinished(std::vector<std::shared_ptr<SchedulerQueue>> queues);

#endif