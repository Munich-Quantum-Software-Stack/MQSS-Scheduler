#include <ScatterSearch.hpp>

namespace milq
{

std::random_device RANDOM_DEVICE;
std::mt19937 RANDOM_GENERATOR(RANDOM_DEVICE());

std::vector<Schedule> initialize_population(std::vector<JobProxy> &proxies,
                                            std::vector<QDMI_Device> *devices)
{
    // TODO: this assumes that jobs are cut appropriately already
    // If this is not the case we should cut before we create the proxies
    // -> convert_to_proxies should be moved here
    std::vector<Machine *> machines;
    for (auto device : *devices)
    {
        int num_qubits, id = 0;
        Machine machine = {
            .device_ref = device,
            .id = id++,
            .capacity = QDMI_query_qubits_num(device, &num_qubits),
            .buckets = {},
            .makespan = 0.0,
            .queue_length = 0.0, // TODO
        };
        machines.push_back(&machine);
    }
    std::sort(proxies.begin(), proxies.end(),
              [](const JobProxy &a, const JobProxy &b)
              { return a.num_qubits < b.num_qubits; });
    // binpacking as initial solution
    // TODO could replace this with different methods and use scatter search as
    // a metatechnique
    do_bin_packing(proxies, machines);
    return {Schedule(machines)};
}

void swap_jobs(Bucket *bucket1, Bucket *bucket2, int machine1_capacity,
               int machine2_capacity)
{
    std::vector<std::pair<int, int>> candidates;
    int bucket1_capacity = 0;
    for (const auto proxy : bucket1->job_proxies)
    {
        bucket1_capacity += proxy->num_qubits;
    }
    int bucket2_capacity = 0;
    for (const auto proxy : bucket2->job_proxies)
    {
        bucket2_capacity += proxy->num_qubits;
    }
    for (int i = 0; i < bucket1->job_proxies.size(); i++)
    {
        for (int j = 0; j < bucket2->job_proxies.size(); j++)
        {
            const auto num_qubits1 =
                (*std::next(bucket1->job_proxies.begin(), i))->num_qubits;
            const auto num_qubits2 =
                (*std::next(bucket2->job_proxies.begin(), j))->num_qubits;
            if (bucket1_capacity - num_qubits1 + num_qubits2 <=
                    machine1_capacity &&
                bucket2_capacity - num_qubits2 + num_qubits1 <=
                    machine2_capacity)
            {
                candidates.push_back(std::make_pair(i, j));
            }
        }
    }
    if (!candidates.empty())
    {
        std::uniform_int_distribution<int> candidate_distribution(
            0, candidates.size());
        int random_index = candidate_distribution(RANDOM_GENERATOR);
        const auto job1 = std::next(bucket1->job_proxies.begin(),
                                    candidates[random_index].first);
        const auto job2 = std::next(bucket2->job_proxies.begin(),
                                    candidates[random_index].second);
        std::swap(*job1, *job2);
    }
}

std::vector<Schedule> local_search(const std::vector<Schedule> &population)
{
    std::vector<Schedule> local_population = population;
    for (auto &schedule : local_population)
    {
        for (auto machine : schedule.get_machines())
        {
            if (machine->buckets.empty())
            {
                continue;
            }
            std::uniform_int_distribution<int> bucket_distribution(
                0, machine->buckets.size());
            int number_of_swaps =
                5 * (machine->buckets.size() > 1
                         ? bucket_distribution(RANDOM_GENERATOR) + 1
                         : 1);
            for (int i = 0; i < number_of_swaps; i++)
            {
                int idx1 = bucket_distribution(RANDOM_GENERATOR);
                int idx2 = bucket_distribution(RANDOM_GENERATOR);
                if (bucket_distribution(RANDOM_GENERATOR) % 2)
                { // swap buckets
                    std::swap(machine->buckets[idx1], machine->buckets[idx2]);
                }
                else
                {
                    swap_jobs(machine->buckets[idx1], machine->buckets[idx2],
                              machine->capacity, machine->capacity);
                }
            }
        }
    }
    return local_population;
}

std::vector<Schedule> diversify(const std::vector<Schedule> &population)
{
    std::vector<Schedule> local_population = population;
    for (Schedule &schedule : local_population)
    {
        std::uniform_int_distribution<int> machine_distribution(
            0, schedule.get_machines().size());
        int number_of_swaps = 5 * machine_distribution(RANDOM_GENERATOR) + 1;
        for (int i = 0; i < number_of_swaps; i++)
        {
            int idx1 = machine_distribution(RANDOM_GENERATOR);
            int idx2 = machine_distribution(RANDOM_GENERATOR);
            auto machine1 = schedule.get_machines()[idx1];
            auto machine2 = schedule.get_machines()[idx2];
            if (machine1->buckets.empty() || machine2->buckets.empty())
            {
                continue;
            }
            std::uniform_int_distribution<int> machine1_distribution(
                0, machine1->buckets.size());
            std::uniform_int_distribution<int> machine2_distribution(
                0, machine2->buckets.size());
            swap_jobs(
                machine1->buckets[machine1_distribution(RANDOM_GENERATOR)],
                machine2->buckets[machine2_distribution(RANDOM_GENERATOR)],
                machine1->capacity, machine2->capacity);
        }
    }
    return local_population;
}

std::vector<Schedule>
generate_new_solutions(const std::vector<Schedule> &population)
{
    auto local_candidates = local_search(population);
    auto global_candidates = diversify(population);
    local_candidates.insert(local_candidates.end(), global_candidates.begin(),
                            global_candidates.end());
    return local_candidates;
}

std::vector<Schedule> improve_population(std::vector<Schedule> &population)
{
    auto population_copy = population;
    for (auto &schedule : population_copy)
    {
        schedule.evaluate();
        auto worst_machine = *std::max_element(
            schedule.get_machines().begin(), schedule.get_machines().end(),
            [](const Machine *m1, const Machine *m2)
            { return m1->makespan < m2->makespan; });

        std::uniform_int_distribution<int> bucket_distribution(
            0, worst_machine->buckets.size() - 1);
        auto index = bucket_distribution(RANDOM_GENERATOR);
        auto bucket = worst_machine->buckets[index];
        worst_machine->buckets.erase(worst_machine->buckets.begin() + index);
        for (auto &job : bucket->job_proxies)
        {
            auto &machine = *std::min_element(
                schedule.get_machines().begin(), schedule.get_machines().end(),
                [](const Machine *m1, const Machine *m2)
                { return m1->capacity < m2->capacity; });
            if (machine->buckets.empty())
            {
                // TODO proprose new cuts ore put them in multipile
                // buckets if necessary
                machine->buckets.push_back(bucket);
            }
            else
            {
                auto &smallest_bucket = *std::min_element(
                    machine->buckets.begin(), machine->buckets.end(),
                    [&machine](const Bucket *b1, const Bucket *b2)
                    {
                        return remaining_capacity(b1, machine->capacity) <
                               remaining_capacity(b2, machine->capacity);
                    });
                if (remaining_capacity(smallest_bucket, machine->capacity) <
                    job->num_qubits)
                {
                    // TODO proprose new cuts ore put them in multipile
                    // buckets if necessary
                    machine->buckets.push_back(bucket);
                }
                else
                {
                    smallest_bucket->job_proxies.push_back(job);
                }
            }
        }
    }
    return population;
}

std::vector<Schedule>
merge_populations(std::initializer_list<std::vector<Schedule>> args)
{
    std::vector<Schedule> combined_solution;
    for (const auto &population : args)
    {
        for (const auto &schedule : population)
        {
            if (std::find(combined_solution.begin(), combined_solution.end(),
                          schedule) == combined_solution.end())
            {
                combined_solution.push_back(schedule);
            }
        }
    }
    return combined_solution;
}

std::vector<Schedule> select_elite(std::vector<Schedule> &population,
                                   int n_solutions)
{
    for (auto &schedule : population)
    {
        schedule.evaluate();
    }
    std::sort(population.begin(), population.end(),
              [](const Schedule &x, const Schedule &y)
              { return x.get_makespan() < y.get_makespan(); });
    return std::vector<Schedule>(population.begin(),
                                 population.begin() + n_solutions);
}

std::vector<Schedule> select_diverse(std::vector<Schedule> &population,
                                     int n_solutions)
{
    std::vector<Schedule> sortedPopulation = population;

    std::sort(sortedPopulation.begin(), sortedPopulation.end(),
              [&population](const Schedule &x, const Schedule &y)
              {
                  return std::accumulate(
                             population.begin(), population.end(), 0,
                             [&x](int sum, Schedule &other)
                             { return sum + x.hamming_distance(other); }) <
                         std::accumulate(
                             population.begin(), population.end(), 0,
                             [&y](int sum, Schedule &other)
                             { return sum + y.hamming_distance(other); });
              });

    return std::vector<Schedule>(sortedPopulation.end() - n_solutions,
                                 sortedPopulation.end());
}

Schedule select_best_solution(std::vector<Schedule> &population)
{

    for (auto &solution : select_elite(population, population.size()))
    {
        if (solution.is_feasible())
        {
            return solution;
        }
    }

    return population[0];
}

Schedule scatter_search(std::vector<JobProxy> &proxies,
                        std::vector<QDMI_Device> *devices, int n_iterations,
                        int n_solutions)
{
    auto population = initialize_population(proxies, devices);
    auto best_schedule = select_best_solution(population);
#pragma omp declare reduction(                                                 \
    min_val:double                                                             \
    : omp_out = [](omp_out, omp_in) {                                          \
        return omp_in.makespan < omp_out->makespan ? omp_in : omp_out;         \
    })
#pragma omp parallel for reduction(min_val : best_schedule)
    for (size_t i = 0; i < n_iterations; i++)
    {
        auto new_population = generate_new_solutions(population);
        auto improved_population = improve_population(population);
        population = merge_populations({population, improved_population});

        auto elite_population = select_elite(population, n_solutions);
        auto diverse_population = select_diverse(population, n_solutions);
        population = merge_populations({elite_population, diverse_population});

        auto current_best = select_best_solution(population);
        best_schedule =
            current_best.get_makespan() < best_schedule.get_makespan()
                ? current_best
                : best_schedule;
    }
    return best_schedule;
}

} // namespace milq
