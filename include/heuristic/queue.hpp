#ifndef SUBMITTER_H
#define SUBMITTER_H

#include <cstddef>
#include <qdmi.h>
#include <queue>
#include <thread>
#include <QuantumTask.hpp>

class SchedulerQueue{

    private: 
    
        double mTotalDuration = 0;
        std::deque<QuantumTask *> tasks = {};

    public:

        int addTask(QuantumTask* pQuantumTask, int position);
        int removeTask(QuantumTask* pQuantumTask);
        double getTotalDuration();
};

#endif