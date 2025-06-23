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

#include <scheduler/queue/queue.hpp>

/**
 * @brief The SchedulerQueue constructor and destructor, responsible for creating the
 * SchedulerQueue object and initializing the queue.
 *
 * Once initialized, the SchedulerQueue will keep jobs queued from MQP and HPCQC
 * and be waited for further scheduling.
 */
SchedulerQueue::SchedulerQueue() {
	std::random_device rd;      	// obtain randomization from hardware
    std::mt19937 gen(rd());     	// seed the generator
    std::uniform_int_distribution<> distr(1000, 9999); // define the range
	sched_queue_ID = distr(gen);	// Initialize the queue ID
	num_total_jobs = 0; // Initialize the total number of jobs
	num_mqp_jobs   = 0; // Initialize the number of MQP jobs
	num_hpcqc_jobs = 0; // Initialize the number of HPCQC jobs
	std::cout << "[LOG-SCHEDULERQUEUE] Constructor: "
			  << "sched_queue_ID=" << sched_queue_ID
			  << ", num_total_jobs=" << num_total_jobs
			  << ", num_mqp_jobs=" << num_mqp_jobs
			  << ", num_hpcqc_jobs=" << num_hpcqc_jobs
			  << std::endl;
}
SchedulerQueue::~SchedulerQueue() { }

/**
 * @brief Function adding jobs to the Scheduler queue
 *
 * This method will handle queueing jobs to the queue. It will check the job and
 * the queue status.
 *
 * @param qjob The job to be handled
 * @return The job queueing status
 */
int SchedulerQueue::addJob(std::shared_ptr<QuantumTask> qjob) {

	// Check the job valid or invalid
	if (qjob == nullptr) {
		std::cout << "[LOG-SCHEDULERQUEUE] Error: qjob is nullptr"
            	  << std::endl;
		exit(1);
	} else {
		jobs.push_back(qjob);
	}

	// Update the queue size
	int queue_size = jobs.size();
	num_total_jobs = queue_size;

	/*
	if (position < 0 || position > this->mTasks.size()) {
	throw std::out_of_range("[LOG-SCHEDULERQUEUE] Position is out of range");
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

	// If we run out of numbers, throw a warning
	if (nextTask->mExecutionOrder == 0) {
		std::cerr << "[LOG-SCHEDULERQUEUE] Ran out of execution order numbers"
				<< std::endl;
	}
	newExecutionOrder = nextTask->mExecutionOrder / 2;
	}

	// If the task is inserted in the middle of the (non-empty) queue
	else {
	std::shared_ptr<QuantumTask> prevTask = this->mTasks[position - 1];
	std::shared_ptr<QuantumTask> nextTask = this->mTasks[position];
	newExecutionOrder =
		(prevTask->mExecutionOrder + nextTask->mExecutionOrder) / 2;

	// If we run out of numbers, throw a warning
	if (newExecutionOrder == prevTask->mExecutionOrder) {
		std::cerr << "[LOG-SCHEDULERQUEUE] Ran out of execution order numbers"
				<< std::endl;
	}
  }

  // Set the execution order of the new task
  quantumTask->mExecutionOrder = newExecutionOrder;

  // Insert the task at the specified position in the queue
  this->mTasks.insert(this->mTasks.begin() + position, quantumTask);

  // Update the total duration of the queue
  this->mTotalDuration += quantumTask->mDuration;
  */

  return 0;
}

/**
 * @brief Function getting jobs from the Scheduler queue
 *
 * This method will handle getting the job object from the queue for further processing.
 * In the case, this job is scheduled to the Submitter for further offloading to
 * be executed on the QDMI device.
 *
 * @param qjob_id The job to be handled
 * @return The job object
 */
int SchedulerQueue::getJob(int job_id, std::shared_ptr<QuantumTask> &ret_job){
	std::cout << "[LOG-SCHEDULERQUEUE] Dequeuing and getting Job"
              << std::endl;
	// find job and get the job in the scheduler queue
	for (auto it = jobs.begin(); it != jobs.end(); ++it) {
		if ((*it)->TaskId == job_id) {
			ret_job = *it; // Assign the found job to ret_job
			std::cout << "[LOG-SCHEDULERQUEUE] Job found: ID = "
						<< ret_job->TaskId << std::endl;
			// Remove the job from the queue
			jobs.erase(it);
			// Update the total number of jobs
			num_total_jobs = jobs.size();
			return 0;
		}
	}
	ret_job = nullptr;
	return -1;
}


/**
 * @brief Function removing jobs from the Scheduler queue
 *
 * This method will handle removing jobs in the queue. In the case, that job is requested to be 
 * remove and no need to be executed.
 *
 * @param qjob The job to be removed
 * @return The job removing status
 */
int SchedulerQueue::removeJob(std::shared_ptr<QuantumTask> qjob) {

	std::cout << "[LOG-SCHEDULERQUEUE] Removing Job from The Queue"
              << std::endl;

	// Find the job in the queue
	auto it = std::find(jobs.begin(), jobs.end(), qjob);
	
	// if the job is found and not found
	if (it != jobs.end()) {
		std::cerr << "[LOG-SCHEDULERQUEUE] Job found: ID = "
				  << (*it)->TaskId
				  << std::endl;
		jobs.erase(it);
		// update the total number of jobs
		num_total_jobs = jobs.size();
	} else {
		std::cerr << "[LOG-SCHEDULERQUEUE] Job not found: "
				  << std::endl;
		return -1;
	}

	/*
	// Find the task in the queue
	auto it = std::find(this->mTasks.begin(), this->mTasks.end(), quantumTask);

	// If the task is not in the queue
	if (it == this->mTasks.end()) {
	std::cerr << " [SchedulerQueue]......Task " << quantumTask->mTaskId
				<< " not found in SchedulerQueue within tasks: ";
	for (auto task : this->mTasks) {
		std::cerr << task->mTaskId << " ";
	}
	std::cerr << std::endl;
	return -1;
	}

	// Remove the task from the queue
	this->mTasks.erase(it);

	// Update the total duration of the queue
	this->mTotalDuration -= quantumTask->mDuration;
	*/

	return 0;
}
