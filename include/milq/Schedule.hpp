#ifndef MILQ_SCHEDULE_HPP
#define MILQ_SCHEDULE_HPP

#include <Estimate.hpp>
#include <list>
#include <numeric>
#include <qdmi.h>
#include <string>
#include <vector>
namespace milq
{

// should only be used as const
// contains the estimates
struct JobProxy
{

    int id;
    int parent_id;
    double processing_time;
    int num_qubits;
    // std::vector<int> indices; 
    // TODO -> supposed to track which were the original qubit indices
    int n_shots;    // = 1024;
    double noise;   // = 0.0;
    int priority;   // = 1;
    int strictness; // = 1;
    std::string preffered_qpu;
};

struct MakespanInfo
{
    JobProxy *proxy;
    double start_time;
    double completion_time;
};

struct Bucket
{
    // A bucket contains all circuits that are going to run on the same machine
    // to approximately the same time

    // List, because we are going to swap elements
    // TODO Instead of sorting this we could sort on insert
    std::list<JobProxy *> job_proxies;
    // std::list<QuantumCircuit> circuits;

    bool operator==(const Bucket &other) const;
};

int remaining_capacity(const Bucket *bucket, const int machine_capacity);

struct Machine
{
    QDMI_Device device_ref;
    int id; // TODO
    int capacity;
    // rarely swap
    std::vector<Bucket *> buckets;
    double makespan;     // = 0.0;
    double queue_length; // = 0.0;

    bool operator==(const Machine &other) const;
};

class Schedule
{
  public:
    Schedule(const std::vector<Machine *> &machines);
    Schedule(const Schedule &other) = default;
    Schedule &operator=(const Schedule &other)
    {
        if (this != &other)
        {
            // TODO is a shallow copy enough?
            machines = other.get_machines();
        }
        return *this;
    }
    // Schedule &operator=(const Schedule &other) = default;
    // Schedule &operator=(Schedule &&other) = default;
    virtual ~Schedule() = default;

    bool operator==(const Schedule &other) const;
    bool is_feasible() const;
    void evaluate();
    int hamming_distance(const Schedule &other) const;

    std::vector<Machine *> get_machines() const { return machines; }
    double get_makespan() const { return makespan; }
    double get_noise() const { return noise; }

  private:
    std::vector<Machine *> machines;
    double makespan;
    double noise;
};

} // namespace milq

#endif // MILQ_SCHEDULE_HPP
