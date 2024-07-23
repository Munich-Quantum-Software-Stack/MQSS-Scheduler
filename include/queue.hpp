#ifndef SUBMITTER_H
#define SUBMITTER_H

#include "Submitter.hpp"
#include <cstddef>
#include <memory>
#include <qdmi.h>
#include <queue>
#include <thread>
#include <QuantumTask.hpp>

// SchedulerQueue class definition
class SchedulerQueue : public ISubmitterObserver {
    private: 
        // Pointer to a Submitter object
        std::shared_ptr<Submitter> mpSubmitter;

    public:
        // Method to add a task at a specific position in the queue
        int addTask(std::shared_ptr<QuantumTask> pQuantumTask, int position);

        // Method to remove a specific task from the queue
        int removeTask(std::shared_ptr<QuantumTask> pQuantumTask);

        // Total duration of all tasks in the queue
        double mTotalDuration = 0;

        // Deque to hold the tasks in the queue
        std::deque<std::shared_ptr<QuantumTask>> mTasks = {};

        // Constructor that takes a pointer to a Submitter object
        explicit SchedulerQueue(std::shared_ptr<Submitter> pSubmitter) : mpSubmitter(pSubmitter) {}

        // Method to update the SchedulerQueue state when a task is popped from the Submitter's queue
        void onTaskPopped(std::shared_ptr<QuantumTask> task) override {
            removeTask(task);
        }
};

#endif