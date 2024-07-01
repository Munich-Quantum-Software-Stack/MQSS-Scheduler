
#include <qdmi.h>
#include <Submitter.hpp>

typedef std::unordered_map<QDMI_Device, std::shared_ptr<Submitter>> Device2SubmitterType;

extern "C" int scheduler(Device2SubmitterType device2Submitter, std::vector<QuantumTask> *tasks);