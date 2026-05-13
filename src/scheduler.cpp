/*
 * Copyright 2024 - 2026 Munich Quantum Software Stack
 * All rights reserved.
 *
 * Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef SCHEDULER_CPP
#define SCHEDULER_CPP

#include <scheduler/scheduler.hpp>

/**
 * @brief The Scheduler constructor and destructor, responsible for creating the
 * Scheduler object and initializing the Scheduler queue.
 *
 * Once initialized, the Scheduler will keep receiving and processing jobs from MQP and HPCQC
 * and will schedule them to the corresponding Submitter of a QDMI device.
 */
Scheduler::Scheduler() {
    sched_stop = false;
    estimated_total_duration = 0.0;

    // Generate a random ID for the scheduler
    std::random_device rd;                           // obtain randomization from hardware
    std::mt19937 gen(rd());                          // seed the generator
    std::uniform_int_distribution<> distr(100, 999); // define the range
    schedulerID = distr(gen);                        // generate a random ID between 100 and 999
    sched_queue = std::make_shared<SchedulerQueue>();

    spdlog::info("-------------------------------");
    spdlog::info("SCHEDULER: initialized with ID: {}", schedulerID);
    spdlog::info("-------------------------------");
}

Scheduler::~Scheduler() {
    sched_stop = true;
    if (scheduler_thread.joinable()) {
        scheduler_thread.join();
    }
}

/**
 * @brief Function initializing and finalizing the Scheduler thread
 *
 * Once initialized, a corresponding thread/process will be created to always check
 * upcoming jobs from MQP/HPCQC. If the submitted job is determined with a device selection,
 * the Scheduler Selector will skip, otherwise, it will handle device selection.
 */
void Scheduler::initScheduler() {

    scheduler_thread = std::thread([this]() {
        while (!sched_stop) {
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                spdlog::info("SCHEDULER-Thread: {} jobs in the queue", sched_queue->num_total_jobs);
                if (submitters.empty()) {
                    spdlog::info("SCHEDULER-Thread: No Submitters available.");
                } else {
                    for (auto &submitter : submitters) {
                        if (submitter->getQueueSize() > 0) {
                            spdlog::info("SCHEDULER-Thread: Submitter BUSY.");
                        } else {
                            spdlog::info("SCHEDULER-Thread: Submitter IDLE.");
                        }
                    } // Iterate through all submitters
                }
            }
        }
    });
}

void Scheduler::finiScheduler() {
    sched_stop = true;
    if (scheduler_thread.joinable()) {
        scheduler_thread.join();
    }
    sched_queue->jobs.clear(); // Clear the scheduler queue
    sched_queue.reset();       // Reset the shared pointer to the queue
    submitters.clear();        // Clear the list of submitters
    spdlog::info("-------------------------------");
    spdlog::info("SCHEDULER-Thread: finalized successfully.");
    spdlog::info("-------------------------------");
}

/**
 * @brief Function handling jobs to submitters
 *
 * This method will handle the jobs to submitters. It will check the job and
 * submitter, and then schedule the job to a relevant submitter.
 *
 * @param qjob The job to be handled
 * @param submitter The submitter to handle the job
 */
void Scheduler::handleJobs2Submitters(int qjob_id, Submitter *submitter_ptr) {

    spdlog::info("SCHEDULER: handling job to Submitter.");

    // check if the job is valid and exists in the schduler queue
    if (submitter_ptr == nullptr) {
        spdlog::info("SCHEDULER: Error - qjob/submitter does not exist.");
        exit(1);
    } else {
        // check the size of the scheduler queue
        int num_current_jobs = sched_queue->jobs.size();
        if (num_current_jobs == 0) {
            spdlog::info("SCHEDULER: No jobs in the queue.");
            return;
        } else {
            // temporarily pop the job from the queue, this means
            // the job is always popped from the front
            std::shared_ptr<QuantumTask> selected_job;
            sched_queue->getJob(qjob_id, selected_job);
            // enqueue the job to the submitter queue
            submitter_ptr->enqueue(selected_job);
        }
    }
}

/**
 * @brief Function for scheduling jobs using First-Come, First-Served (FCFS) method
 *
 * Jobs are ordered strictly by SubmitTime — the earliest arrival is dispatched
 * first. The deque is sorted in-place and ExecutionOrder is updated to reflect
 * each job's position in the resulting schedule.
 */
void Scheduler::scheduleFCFS() {
    spdlog::info("SCHEDULER: Scheduling Quantum Jobs using FCFS");

    if (sched_queue->jobs.empty()) {
        spdlog::warn("SCHEDULER [FCFS]: Queue is empty, nothing to schedule");
        return;
    }

    // Sort by arrival time — stable to preserve insertion order for ties
    std::stable_sort(sched_queue->jobs.begin(), sched_queue->jobs.end(),
        [](const std::shared_ptr<QuantumTask> &a, const std::shared_ptr<QuantumTask> &b) {
            return a->SubmitTime < b->SubmitTime;
        });

    // Assign execution order based on sorted position
    for (int i = 0; i < static_cast<int>(sched_queue->jobs.size()); i++) {
        sched_queue->jobs[i]->ExecutionOrder = i;
    }

    spdlog::info("SCHEDULER [FCFS]: {} jobs ordered by submit time:", sched_queue->jobs.size());
    for (const auto &job : sched_queue->jobs) {
        spdlog::info("  [order={}] Job {:3d} | submitted: {} | circuit: {}",
                     job->ExecutionOrder, job->TaskId,
                     job->SubmitTime, job->CircuitFile);
    }
}

/**
 * @brief Function for scheduling jobs using Round Robin (RR) method
 *
 * This method will schedule the jobs in the queue using the Round Robin method.
 */
void Scheduler::scheduleRR() {
    spdlog::info("SCHEDULER: Scheduling Quantum Jobs using Round Robin");
}

/**
 * @brief Function for scheduling jobs using Backfilling (BF) method
 *
 * This method will schedule the jobs in the queue using the Backfilling method.
 */
void Scheduler::scheduleBF() {
    spdlog::info("SCHEDULER: Scheduling Quantum Jobs using BF (Backfilling)");
}

/**
 * @brief Function for scheduling jobs using Mix-N-Multi (MNM) method
 *
 * This method will schedule the jobs in the queue using the Mix-N-Multi method.
 */
void Scheduler::scheduleMNM() { spdlog::info("SCHEDULER: Scheduling Quantum Jobs using MNM"); }

#endif // SCHEDULER_CPP