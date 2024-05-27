#ifndef SUBMITTER_H
#define SUBMITTER_H

#include "Submitter.hpp"
#include <cstddef>
#include <qdmi.h>
#include <queue>
#include <thread>
#include <QuantumTask.hpp>

// SchedulerQueue class definition
class SchedulerQueue : public ISubmitterObserver {
    private: 
        // Pointer to a Submitter object
        Submitter *mpSubmitter;

    public:
        // Method to add a task at a specific position in the queue
        int addTask(QuantumTask* pQuantumTask, int position);

        // Method to remove a specific task from the queue
        int removeTask(QuantumTask* pQuantumTask);

        // Total duration of all tasks in the queue
        double mTotalDuration = 0;

        // Deque to hold the tasks in the queue
        std::deque<QuantumTask *> mTasks = {};

        // Constructor that takes a pointer to a Submitter object
        explicit SchedulerQueue(Submitter *pSubmitter) : mpSubmitter(pSubmitter) {}

        // Method to update the SchedulerQueue state when a task is popped from the Submitter's queue
        void onTaskPopped(QuantumTask* task) override {
            removeTask(task);
        }
};

#endif