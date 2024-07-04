#ifndef MILQ_ESTIMATE_HPP
#define MILQ_ESTIMATE_HPP

#include <QuantumResourceManager.hpp>
#include <qdmi.h>
#include <vector>

namespace milq
{
struct JobProxy;

//TODO!! these are not pproperly implemented
double estimate_noise(const QuantumTask &task, QDMI_Device *devices);
double estimate_processing_time(const QuantumTask &task, QDMI_Device *devices);
double estimate_proxy_noise(const JobProxy &proxy, QDMI_Device *devices);
double estimate_proxy_processing_time(const JobProxy &proxy,
                                      QDMI_Device *devices);
} // namespace milq

#endif // MILQ_ESTIMATE_HPP
