/* Scheduler Queues. */

#include <iostream>
#include <queue.hpp>

/*
 * @brief Add a QuantumTask to the SchedulerQueue at a specific position
 * @param quantumTask The QuantumTask to be added
 * @param position The position in the queue where the task should be inserted
 * @return The execution order of the added task
 */
int SchedulerQueue::addTask(std::shared_ptr<QuantumTask> quantumTask,
                            int position) {
  if (quantumTask == nullptr) {
    throw std::invalid_argument(
        "   [SchedulerQueue]......pQuantumTask is a null pointer");
  }

  if (position < 0 || position > this->mTasks.size()) {
    throw std::out_of_range(
        "   [SchedulerQueue]......Position is out of range");
  }

  double newExecutionOrder = 0.0;
  double executionOrderIncrement = 1.0e12;

  // If queue is empty
  if (this->mTasks.size() == 0) {
    newExecutionOrder = executionOrderIncrement;
  }

  // If the task is inserted at the end of the (non-empty) queue
  else if (position == this->mTasks.size()) {
    std::shared_ptr<QuantumTask> prevTask = this->mTasks[position - 1];
    newExecutionOrder = prevTask->mExecutionOrder + executionOrderIncrement;
  }

  // If the task is inserted at the beginning of the (non-empty) queue
  else if (position == 0) {
    std::shared_ptr<QuantumTask> nextTask = this->mTasks[0];

    // Check that we don't run out of numbers
    if (nextTask->mExecutionOrder == 0) {
      throw std::runtime_error(
          "   [SchedulerQueue]......Ran out of execution order numbers");
    }
    newExecutionOrder = nextTask->mExecutionOrder / 2;
  }

  // If the task is inserted in the middle of the (non-empty) queue
  else {
    std::shared_ptr<QuantumTask> prevTask = this->mTasks[position - 1];
    std::shared_ptr<QuantumTask> nextTask = this->mTasks[position];
    newExecutionOrder =
        (prevTask->mExecutionOrder + nextTask->mExecutionOrder) / 2;

    // Check that we don't run out of numbers
    if (newExecutionOrder == prevTask->mExecutionOrder) {
      throw std::runtime_error(
          "   [SchedulerQueue]......Ran out of execution order numbers");
    }
  }

  // Set the execution order of the new task
  quantumTask->mExecutionOrder = newExecutionOrder;

  // Insert the task at the specified position in the queue
  this->mTasks.insert(this->mTasks.begin() + position, quantumTask);

  // Update the total duration of the queue
  this->mTotalDuration += quantumTask->mDuration;

  return newExecutionOrder;
}

int SchedulerQueue::removeTask(std::shared_ptr<QuantumTask> quantumTask) {
  // Find the task in the queue
  auto it = std::find(this->mTasks.begin(), this->mTasks.end(), quantumTask);

  // If the task is not in the queue
  if (it == this->mTasks.end()) {
    std::cerr << "   [SchedulerQueue]......Task " << quantumTask->mTaskId
              << " not found in SchedulerQueue within tasks: ";
    for (auto task : this->mTasks) {
      std::cerr << task->mTaskId << " ";
    }
    std::cerr << std::endl;
    return -1; // Return an error code
  }

  // Remove the task from the queue
  this->mTasks.erase(it);

  // Update the total duration of the queue
  this->mTotalDuration -= quantumTask->mDuration;

  return 0;
}
