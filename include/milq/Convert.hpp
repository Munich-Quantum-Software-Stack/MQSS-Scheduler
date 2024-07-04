#ifndef MILQ_CONVERT_HPP
#define MILQ_CONVERT_HPP

#include <QuantumResourceManager.hpp>
#include <Schedule.hpp>
#include <qdmi.h>
#include <vector>

namespace milq
{
std::vector<JobProxy> convert_to_proxies(const std::vector<QuantumTask> &tasks);
void assign_devices(const milq::Schedule &schedule);

} // namespace milq

#endif // MILQ_CONVERT_HPP
