/**
 * @file scheduler_round_robin.cpp
 * @brief Implementation of a dummy scheduler.
 */

#include <iostream>
#include <round_robin.hpp>
#include "Submitter.hpp"


/**
 * @brief The main entry point of the program.
 *
 * The Scheduler.
 *
 * @return const char *
 */
extern "C" int scheduler(Submiter2Device device2Submitter, std::vector<QuantumTask *> tasks)
{
    // Query the available devices
    if (device2Submitter.size() == 0)
    {
        std::cerr << "   [Scheduler]...........Error: no devices found" << std::endl;
        return 1;
    }

    for (auto &childQuantumTask : tasks)
    {
        QDMI_Device device = device2Submitter.begin()->first;

        const char *libname = "the first lib";
        //strrchr(device->library.libname, '/');

        std::cout << "   [Scheduler]...........Setting "
                  << libname << " as target device "
                  << "for job with ID " << childQuantumTask->mTaskId
                  << std::endl;

        childQuantumTask->mScheduledQpu = device;
    }

    return 0; 
}