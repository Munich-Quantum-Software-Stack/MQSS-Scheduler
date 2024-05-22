/* Scheduler Queues. */

#include "QuantumTask.hpp"
#include <queue.hpp>

/*
 * @brief Extract supermarq+ features from a quantum circuit
 * @param TSM The quantum circuit to evaluate
 * @param gate_counts The counts of each gate in the circuit
 * @return A vector of original supermarq and 3 additional features
 */
int SchedulerQueue::addTask(QuantumTask* pQuantumTask, int position)
{   
    int mNewExecutionOrder = 0;

    // If queue is empty
    if (tasks.size() == 0) {
        mNewExecutionOrder = 1000;
    }

    // If the task is inserted at the end of the (non-empty) queue
    else if (position == tasks.size()) {
        QuantumTask *prevTask = tasks[position];
        mNewExecutionOrder = prevTask->mExecutionOrder + 1000;
    }

    // If the task is inserted at the beginning of the (non-empty) queue
    else if (position == 0) {
        QuantumTask *nextTask = tasks[0];

        // Check that we don't run out of numbers
        if (nextTask->mExecutionOrder == 0) {
            throw std::runtime_error("Ran out of execution order numbers");
        }
        mNewExecutionOrder = nextTask->mExecutionOrder / 2;
    }

    // If the task is inserted in the middle of the (non-empty) queue
    else {
        QuantumTask *prevTask = tasks[position - 1];
        QuantumTask *nextTask = tasks[position];
        mNewExecutionOrder = (prevTask->mExecutionOrder + nextTask->mExecutionOrder) / 2;

        // Check that we don't run out of numbers
        if (mNewExecutionOrder == prevTask->mExecutionOrder) {
            throw std::runtime_error("Ran out of execution order numbers");
        }
    }

    // Set the execution order of the new task
    pQuantumTask->mExecutionOrder = mNewExecutionOrder;

    // Insert the task at the specified position in the queue
    tasks.insert(tasks.begin() + position, pQuantumTask);

    // Update the total duration of the queue
    mTotalDuration += pQuantumTask->mDuration;

    return mNewExecutionOrder;
}

int SchedulerQueue::removeTask(QuantumTask* pQuantumTask)
{
    // Find the task in the queue
    auto it = std::find(tasks.begin(), tasks.end(), pQuantumTask);

    // If the task is not in the queue
    if (it == tasks.end()) {
        return -1;
    }

    // Remove the task from the queue
    tasks.erase(it);

    // Update the total duration of the queue
    mTotalDuration -= pQuantumTask->mDuration;

    return 0;
}

double SchedulerQueue::getTotalDuration()
{
    return mTotalDuration;
}