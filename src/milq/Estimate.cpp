#include <Estimate.hpp>

namespace milq
{
double estimate_noise(const QuantumTask &task, QDMI_Device devices)
{
    // TODO find out what we can use here
    return 0.0;
}

double estimate_processing_time(const QuantumTask &task, QDMI_Device devices)
{
    // TODO find out what we can use here
    return 0.0;
}

double estimate_proxy_noise(const JobProxy &proxy, QDMI_Device devices)
{
    // TODO find out what we can use here
    return 0.0;
}

double estimate_proxy_processing_time(const JobProxy &proxy,
                                      QDMI_Device devices)
{
    // TODO Circuit depth * gate_time
    return 0.0;
}
} // namespace milq
