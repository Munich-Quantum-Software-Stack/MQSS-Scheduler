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

#ifndef SCHEDULER_QUEUE_H
#define SCHEDULER_QUEUE_H

#include <deque>
#include <random>
#include <memory>
#include <iostream>
#include <quantum_task.hpp>
#include <submitter.hpp>

/**
 * @brief The SchedulerQueue class, providing attributes and methods for the queue
 * at the Scheduler side. This queue holds tasks received from MQP (via Cloud) and HPCQC.
 * 
 * To be simple, the SchedulerQueue object has its own methods that might override the functions
 * of the Submitter class.
 */
class SchedulerQueue {

public:

	// ---------------------------------------------------
    // Attributes
    // ---------------------------------------------------
	int sched_queue_ID;
	int num_total_jobs;
	int num_mqp_jobs;
	int num_hpcqc_jobs;
	std::deque<std::shared_ptr<QuantumTask>> jobs = {};

	// ---------------------------------------------------
    // Constructors and Destructors
    // ---------------------------------------------------
	SchedulerQueue();
	~SchedulerQueue();

	// ---------------------------------------------------
    // Methods
    // ---------------------------------------------------
	int addJob(std::shared_ptr<QuantumTask> qjob);
	int getJob(int job_id, std::shared_ptr<QuantumTask> &ret_qjob);
	int removeJob(std::shared_ptr<QuantumTask> qjob);

};

#endif // SCHEDULER_QUEUE_HPP