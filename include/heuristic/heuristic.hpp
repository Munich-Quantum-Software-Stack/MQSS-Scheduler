#include "Submitter.hpp"
#include "qdmi.h"

typedef std::unordered_map<QDMI_Device, std::shared_ptr<Submitter>> Submiter2Device;


extern "C" int scheduler(Submiter2Device device2Submitter, std::vector<QuantumTask *> tasks);
