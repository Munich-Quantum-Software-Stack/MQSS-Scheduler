#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.hpp"
#include <QuantumTask.hpp>
#include <memory>
#include <vector>

extern "C" int
scheduler(std::vector<std::shared_ptr<SchedulerQueue>> SchedulerQueues,
          std::vector<std::shared_ptr<QuantumTask>> tasks);

#endif