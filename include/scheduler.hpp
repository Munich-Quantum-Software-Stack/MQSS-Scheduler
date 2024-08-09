#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.hpp"
#include <memory>
#include <quantum_task.hpp>
#include <vector>

extern "C" int
scheduler(std::vector<std::shared_ptr<SchedulerQueue>> SchedulerQueues,
          std::vector<std::shared_ptr<QuantumTask>> tasks);

#endif