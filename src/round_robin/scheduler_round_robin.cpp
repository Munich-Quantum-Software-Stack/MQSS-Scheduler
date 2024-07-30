/**
 * @file scheduler_round_robin.cpp
 * @brief Implementation of a dummy scheduler.
 */

#include "queue.hpp"
#include <Submitter.hpp>
#include <iostream>
#include <round_robin.hpp>

/**
 * @brief The main entry point of the program.
 *
 * The Scheduler.
 *
 * @return const char *
 */
extern "C" int
scheduler(std::vector<std::shared_ptr<SchedulerQueue>> SchedulerQueues,
          std::vector<std::shared_ptr<QuantumTask>> tasks) {
  // Query the available devices

  if (SchedulerQueues.size() == 0) {

    std::cout << "   [Scheduler]...........Error: no devices found"
              << std::endl;
    return 1;
  }

  std::cout << "   [Scheduler]..........." << SchedulerQueues.size()
            << " available device(s)" << std::endl;

  int idx = 0; // Round-robin device index
  for (auto &childQuantumTask : tasks) {
    QDMI_Device device = SchedulerQueues[idx]->mpSubmitter->mDevice;

    const char *libname = "the first lib";
    // strrchr(device->library.libname, '/');

    std::cout << "   [Scheduler]...........Setting " << libname
              << " as target device "
              << "for job with ID " << childQuantumTask->mTaskId << std::endl;

    int queueSize = SchedulerQueues[idx]->mTasks.size();

    childQuantumTask->mScheduledQpu = device;
    childQuantumTask->mExecutionOrder =
        SchedulerQueues[idx]->addTask(childQuantumTask, queueSize);

    idx = (idx + 1) % SchedulerQueues.size();
  }

  return 0;
}