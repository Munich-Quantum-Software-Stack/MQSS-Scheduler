#include "Submitter.hpp"
#include "queue.hpp"
#include "qdmi.h"

extern "C" int scheduler(std::vector<std::shared_ptr<SchedulerQueue>> SchedulerQueues, std::vector<std::shared_ptr<QuantumTask>> tasks);
