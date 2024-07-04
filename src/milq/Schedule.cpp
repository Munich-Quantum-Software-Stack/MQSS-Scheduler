#include <algorithm>
#include <list>
#include <numeric>
#include <string>
#include <vector>

#include <Schedule.hpp>

namespace milq
{
/*
 * Free functions
 */
double makespan_function(const std::vector<MakespanInfo *> &job_infos,
                         const std::string machine, double alpha = 1.0,
                         double beta = 1.0)
{
    // Alpha and beta are weights for the priority and strictness of the jobs
    double makespan = 0.0;
    for (const auto &info : job_infos)
    {
        makespan += info->completion_time * info->proxy->priority * alpha;
        makespan += (info->proxy->preffered_qpu == machine)
                        ? 0
                        : info->proxy->strictness * beta;
    }
    return makespan;
}

std::vector<int> flattened_proxy_ids(const Machine &machine)
{
    std::vector<int> result;
    for (const auto &bucket : machine.buckets)
    {
        for (const auto &proxy : bucket->job_proxies)
        {
            result.push_back(proxy->id);
        }
    }
    return result;
}

double calculate_proxy_makespan(const std::vector<Bucket *> &buckets,
                                const std::string machine_id,
                                const double gamma = 1.0,
                                const double delta = 10.0)
{
    // Gamma and delta are the set up times coifficients
    // Gamma is the set up time for the similar circuits, e.g. parametrized

    // Set up initial values sorted by the buckets
    std::vector<MakespanInfo *> proxy_infos;
    for (size_t idx = 0; idx < buckets.size(); ++idx)
    {
        const auto &bucket = buckets[idx];
        for (const auto &proxy : bucket->job_proxies)
        {
            auto *info = new MakespanInfo{proxy, 0.0, 0.0};
            proxy_infos.push_back(info); //
        }
    }
    if (proxy_infos.empty())
    {
        return 0.0;
    }
    // assigned
    std::vector<MakespanInfo *> processed_proxies = proxy_infos;
    for (auto &proxy_info : proxy_infos)
    {
        // find predecessor
        auto predecessor = **std::max_element(
            processed_proxies.begin(), processed_proxies.end(),
            [](const MakespanInfo *a, const MakespanInfo *b)
            { return a->completion_time < b->completion_time; });
        // Start of iteration: Dummy Job
        if (proxy_info->start_time == 0.0)
        {
            predecessor = MakespanInfo{nullptr, 0.0, 0.0};
        }
        proxy_info->start_time = predecessor.completion_time;
        // calculate completion time
        double set_up_time = delta;
        if (predecessor.proxy != nullptr &&
            proxy_info->proxy->parent_id == predecessor.proxy->parent_id) //&&
        // proxy_info->proxy->indices == predecessor.proxy->indices) TODO check
        // if weare the same partion
        {
            set_up_time *= gamma;
        }

        proxy_info->completion_time = predecessor.completion_time +
                                      proxy_info->proxy->processing_time +
                                      set_up_time;
    }
    return makespan_function(proxy_infos, machine_id);
}

double calculate_proxy_noise(const std::vector<Bucket *> &buckets)
{
    double total_noise = 0.0;
    for (const auto &bucket : buckets)
    {
        for (const auto &proxy : bucket->job_proxies)
        {
            total_noise += proxy->noise;
        }
    }
    return total_noise;
}

int remaining_capacity(const Bucket *bucket, const int machine_capacity)
{
    int total_num_qubits = std::accumulate(
        bucket->job_proxies.begin(), bucket->job_proxies.end(), 0,
        [](int sum, const JobProxy *proxy) { return sum + proxy->num_qubits; });

    return machine_capacity - total_num_qubits;
}

/*
 * Bucket
 */
bool Bucket::operator==(const Bucket &other) const
{
    if (job_proxies.size() != other.job_proxies.size())
        return false;

    return std::equal(job_proxies.begin(), job_proxies.end(),
                      other.job_proxies.begin());
};

/*
 * Machine
 */
bool Machine::operator==(const Machine &other) const
{
    if (id != other.id || buckets.size() != other.buckets.size())
        return false;

    // order matters here so no sorting
    return std::equal(buckets.begin(), buckets.end(), other.buckets.begin());
};

/*
 * Schedule
 */
Schedule::Schedule(const std::vector<Machine *> &machines) : machines(machines)
{
    makespan = 0.0;
    noise = 0.0;
}

bool Schedule::operator==(const Schedule &other) const
{
    if (machines.size() != other.machines.size())
        return false;
    return std::equal(machines.begin(), machines.end(), other.machines.begin());
}

bool Schedule::is_feasible() const
{
    for (const auto &machine : machines)
    {
        for (const auto &bucket : machine->buckets)
        {
            int total_num_qubits = std::accumulate(
                bucket->job_proxies.begin(), bucket->job_proxies.end(), 0,
                [](int sum, const JobProxy *proxy)
                { return sum + proxy->num_qubits; });

            if (total_num_qubits > machine->capacity)
                return false;
        }
    }

    return true;
}

void Schedule::evaluate()
{
    std::vector<double> makespans;
    std::vector<double> noises;

    for (const auto &machine : machines)
    {
        double proxy_makespan =
            machine->queue_length +
            calculate_proxy_makespan(machine->buckets,
                                     std::to_string(machine->id)); // TODO
        makespans.push_back(proxy_makespan);
        machine->makespan = proxy_makespan;
        noises.push_back(calculate_proxy_noise(machine->buckets));
    }

    makespan = *std::max_element(makespans.begin(), makespans.end());
    noise = std::accumulate(noises.begin(), noises.end(), 0.0);
}

int Schedule::hamming_distance(const Schedule &other) const
{
    // Hamming distance like metric, assumes both schedules have the same number
    // of machines
    int distance = 0;

    for (size_t i = 0; i < machines.size(); i++)
    {
        const Machine &machine1 = *machines[i];
        const Machine &machine2 = *other.machines[i];
        int num_buckets =
            std::max(machine1.buckets.size(), machine2.buckets.size());

        auto proxies1 = flattened_proxy_ids(machine1);
        auto proxies2 = flattened_proxy_ids(machine2);

        for (size_t j = 0; j < proxies1.size(); j++)
        {
            auto k = std::find(proxies2.begin(), proxies2.end(), proxies1[j]);
            if (k != proxies2.end())
            {
                distance += std::abs(static_cast<int>(j) -
                                     std::distance(proxies2.begin(), k));
            }
            else
            {
                distance += num_buckets;
            }
        }
    }

    return distance;
}

} // namespace milq
