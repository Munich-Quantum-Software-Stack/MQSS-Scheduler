#include "queue.hpp"
#include <memory>
#include <qdmi.h>
#include <quantum_task.hpp>
#include <unordered_map>
#include <vector>

std::unordered_map<QDMI_Device, float>
calculate_scores(std::shared_ptr<QuantumTask> task,
                 std::vector<QDMI_Device> availableDevices);

QDMI_Device
choose_device(std::shared_ptr<QuantumTask> task,
              const std::unordered_map<QDMI_Device, float> &scores,
              std::vector<std::shared_ptr<SchedulerQueue>> schedulerQueues);

int backfilling(std::shared_ptr<QuantumTask> pNewTask,
                std::shared_ptr<SchedulerQueue> pQueue);