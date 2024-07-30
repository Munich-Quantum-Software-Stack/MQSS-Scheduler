#include "Submitter.hpp"
#include "qdmi.h"
#include "queue.hpp"

extern "C" int
scheduler(std::vector<std::shared_ptr<SchedulerQueue>> SchedulerQueues,
          std::vector<std::shared_ptr<QuantumTask>> tasks);
