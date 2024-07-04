#include <Convert.hpp>

namespace milq
{

std::vector<JobProxy> convert_to_proxies(const std::vector<QuantumTask> &tasks)
{
    std::vector<JobProxy> proxies;
    for (const auto &task : tasks)
    {
        if (NULL != task.scheduled_qpu)
        {
            continue;
        }
        /*
        TODO noise and processing time get estimated
        Once schedule is initialized
        */
        JobProxy proxy;
        proxy.id = task.task_id;
        proxy.parent_id = task.parent_id;
        proxy.processing_time = 0;
        proxy.num_qubits = task.n_qbits;
        proxy.n_shots = task.n_shots;
        proxy.noise = 0;
        proxy.priority = task.priority;
        proxy.strictness = 1; // TODO
        proxy.preffered_qpu = task.preferred_qpu;
        proxies.push_back(proxy);
    }
    return proxies;
}
} // namespace milq
