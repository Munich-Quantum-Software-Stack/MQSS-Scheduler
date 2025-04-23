#include "scheduler.hpp"
#include "queue.hpp"
#include <memory>
#include <qdmi/client.h>
#include <quantum_task.hpp>
#include <unordered_map>
#include <vector>

int scheduler(std::vector<std::shared_ptr<SchedulerQueue>> schedulerQueues,
    std::vector<std::shared_ptr<QuantumTask>> tasks);
