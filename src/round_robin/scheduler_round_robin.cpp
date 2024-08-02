/**
 * @file scheduler_round_robin.cpp
 * @brief Implementation of a simple round robin scheduler.
 */

#include "queue.hpp"
#include "round_robin.hpp"

/**
 * @brief The main entry point of the program.
 *
 * A round robin scheduler that assigns tasks by looping through the available
 * devices.
 *
 * @return 0 once all tasks have been scheduled.
 */
extern "C" int
scheduler(std::vector<std::shared_ptr<SchedulerQueue>> schedulerQueues,
          std::vector<std::shared_ptr<QuantumTask>> tasks) {
  int idx = 0; // Round-robin device index

  for (auto &childQuantumTask : tasks) {

    // Assign the idx device to the task
    childQuantumTask->mScheduledQpu =
        schedulerQueues[idx]->mpSubmitter->mDevice;

    // Add task to the end of the queue
    int queueSize = schedulerQueues[idx]->mTasks.size();
    childQuantumTask->mExecutionOrder =
        schedulerQueues[idx]->addTask(childQuantumTask, queueSize);

    // Increment the index for next task
    idx = (idx + 1) % schedulerQueues.size();
  }

  return 0;
}