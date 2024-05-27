
#include "fomac.hpp"
#include  <Submitter.hpp>
#include "queue.hpp"
#include <qdmi.h>

typedef std::unordered_map<My_QDMI_Device, std::shared_ptr<Submitter>> Submitter2Device;
typedef std::unordered_map<My_QDMI_Device, std::shared_ptr<SchedulerQueue>> Scheduler2Device;


extern "C" int scheduler(Scheduler2Device device2SchedQueue, std::vector<QuantumTask *> tasks);
