/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <random>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <quantum_task.hpp>

#include "queue/queue.hpp"
#include "utils/predictor.hpp"


/**
 * @brief The Scheduler class, prividing attributes and methods regarding
 * scheduling quantum tasks to a submitter of a corresponding QDMI device.
 * 
 * To be simple, the Scheduler object has its own queue for receiving quantum tasks
 * from MQP (via Cloud) or HPQC (via HPC systems). Then, for tasks without device & pass
 * selection prefernce, the module called selector/predictor in Scheduler will handle a
 * good selection. Afterwards, the waiting tasks will be scheduled further to a corresponding
 * Submitter of a QDMI device.
 */
class Scheduler {

public:
    // ---------------------------------------------------
    // Attributes
    // ---------------------------------------------------
    int schedulerID;    // Identifier of The Scheduler
    bool sched_stop;    // The stop flag
    double estimated_total_duration;            // Estimated Time of Tasks in The Scheduler Queue
    std::condition_variable sched_condition;    // The condition for the queue lock
    std::thread scheduler_thread;               // The thread for the submitter
    std::shared_ptr<SchedulerQueue> sched_queue;        // The Scheduler Queue
    std::vector<std::shared_ptr<Submitter>> submitters; // List of Submitters

    // ---------------------------------------------------
    // Constructor
    // ---------------------------------------------------
    Scheduler();
    ~Scheduler();

    // ---------------------------------------------------
    // Methods
    // ---------------------------------------------------
    void initScheduler();
    void finiScheduler();
    void handleJobs2Submitters(int job_id, Submitter* submitter_ptr);
    void scheduleFCFS();    // First-Come, First-Served scheduling
    void scheduleRR();      // Round Robin scheduling
    void scheduleBF();      // Backfilling scheduling
    void scheduleMNM();     // Mix-N-Multi scheduling

};

#endif // SCHEDULER_HPP
