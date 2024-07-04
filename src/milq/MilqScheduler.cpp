/**
 * @file scheduler_round_robin.cpp
 * @brief Implementation of a dummy scheduler.
 */

#include <iostream>
#include <string>
#include <vector>

#include <PassModule.hpp>
#include <QuantumResourceManager.hpp>

#include <Convert.hpp>
#include <ScatterSearch.hpp>
#include <Schedule.hpp>
#include <fomac.hpp>
#include <qdmi.h>

struct QuantumTask;
std::vector<QuantumTask> FOMAC_query_queue(QDMI_Device device) { return {}; };
int MILQ_MAX_PRIORITY = 19;
/**
 * @brief The main entry point of the program.
 *
 * The Scheduler.
 *
 * @return const char *
 */

extern "C" int scheduler(std::vector<QuantumTask> *childQuantumTasks)
{
    // TODO: load configuration
    // Query the available devices
    std::vector<QDMI_Device> devices =
        FOMAC_available_devices(false /*verbose*/);

    if (devices.size() == 0)
    {
        std::cout << "   [Scheduler]...........Error: no devices found"
                  << std::endl;
        return 1;
    }

    std::cout << "   [Scheduler]..........." << devices.size()
              << " available device(s)" << std::endl;
    // 1. Priority Jobs & Backfilling
    for (auto &task : *childQuantumTasks)
    {
        // TODO: Is there a difference between hybrid and pure jobs?
        // 1.1. Priority Jobs
        if (task.priority >= MILQ_MAX_PRIORITY)
        {
            // TODO: decide if we want to halt running jobs!
            QDMI_Device target_device = NULL;

            for (auto device : devices)
            {
                const char *device_libname =
                    strrchr(device->library.libname, '/');

                if (device_libname == task.preferred_qpu)
                {
                    target_device = device;
                    task.scheduled_qpu = target_device;
                    break;
                }
            }
        }
        // 2. Backfilling
        for (auto device : devices)
        {
            // 2.1 query queue
            std::vector<QuantumTask> queue = FOMAC_query_queue(device); // TODO
            for (size_t i = 0; i < queue.size(); i++)
            {
                auto queued_task = &queue.at(i);
                int num_qubits; // TODO
                if (task.n_qbits + queued_task->n_qbits <=
                    QDMI_query_qubits_num(device, &num_qubits))
                { // TODO
                    QDMI_Device target_device = NULL;
                    const char *device_libname =
                        strrchr(device->library.libname, '/');

                    if (device_libname == task.preferred_qpu)
                    {
                        target_device = device;
                        task.scheduled_qpu = target_device;
                        // queue.at(i).update(); TODO: merge new tasks, keep
                        // track what was merged
                        break;
                    }
                }
            }
        }
    }

    // 3. Convert to proxies
    std::vector<milq::JobProxy> job_proxies =
        milq::convert_to_proxies(*childQuantumTasks);
    // 4. Scatter Search
    auto schedule = milq::scatter_search(job_proxies, &devices);
    // 5. Convert back to jobs
    milq::assign_devices(schedule);

    return 0;
}
