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

#ifndef QUANTUMTASK_CPP
#define QUANTUMTASK_CPP

#include "submitter.hpp"
#include "quantum_task.hpp"

#include <iostream>
#include <unistd.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_ostream.h>

#include <qdmi.hpp>
#include <qdmi/constants.h>
#include <qinfo/qinfo.h>

using namespace llvm;

/**
 * @brief The Submitter class, responsible for submitting tasks to the QDMI
 *
 * Once initialized, the Submitter will keep the device busy with a maximum of
 * `num_threads` that can be submitted in parallel and will be submitted
 * according to their execution order.
 *
 * @param device The QDMI device to submit quantum tasks
 * @param num_threads The number of threads is used for submitting tasks
 */
Submitter::Submitter() { }
Submitter::Submitter(qdmi::Device device) {
  device_name = device.get_name();
}

/**
 * @brief Destructor for the Submitter class
 *
 * Once destroyed, the Submitter will stop all threads and wait for 
 * them to finish before exiting.
 */
Submitter::~Submitter() { 
  stop = true;
  if (submitter_thread.joinable()) {
      submitter_thread.join();
  }
}

/**
 * @brief Function initializing the Submitter for a specific QDMI device
 *
 * Once initialized, a corresponding thread will be created to always check
 * the queue. If tasks are available, and the corresponding QDMI device is ready
 * for executing tasks, tasks can be submitted to the device.
 * 
 * @param device Check the queue of this device.
 */
void Submitter::initSubmitter(qdmi::Device device){
  device_name = device.get_name().c_str();
  submitter_thread = std::thread([this]() {
    while (!stop) {
      {
        std::cout << "[LOG-SUBMITTER] Submitter Thread Checking Queue ..." << std::endl;
        if (!sub_queue.empty()) {
          std::cout << "-------------------------" << std::endl;
          std::cout << "Tasks Availabl           " << std::endl;
          std::cout << "-------------------------" << std::endl;
          std::cout << "Checking QDMI Device ... " << std::endl;
          std::cout << "-------------------------" << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(250));
        } else {
          std::cout << "-------------------------" << std::endl;
          std::cout << "Empty Queue              " << std::endl;
          std::cout << "-------------------------" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
  });
}

/**
 * @brief Function finalizing the Submitter for a specific QDMI device
 *
 * Once finalized, the corresponding thread will be stoped and destroyed. Before finalizing,
 * the Submitter needs to check running tasks or waiting tasks in the queue
 * 
 * @param device Check the queue of this device.
 */
void Submitter::finiSubmitter(qdmi::Device device) {
  device_name = device.get_name().c_str();
  std::cout << "[LOG-SUBMITTER] Finalizing Submitter for device: " << device_name << std::endl;
  stop = true;
  if (submitter_thread.joinable()) {
    submitter_thread.join();
  }
}

/**
 * @brief Function enqueuing tasks to the submit queue
 *
 * Once enqueued, the task will be scheduled and submitted to the QDMI device
 * for executing. The task should be a valid QuantumTask object.
 * 
 * @param task a pointer referring to the QuantumTask object
 */
void Submitter::enqueue(std::shared_ptr<QuantumTask> task) {
  // Now each submitter manages a device, and each submitter has
  // its own queue and creates its own thread to manage the queue.
  // Therefore, this context doesn't need a global lock. For the case of a submitter
  // supports multiple threads, we can use a mutex to protect the queue.
  // std::unique_lock<std::mutex> lock(qlock);
  if (stop)
    throw std::runtime_error("Error: enqueue called on a stopped Submitter");
  else
    sub_queue.push(task);
}

/**
 * @brief Function getting the submitter queue size
 *
 * This method should return the number of tasks in the queue. It is used to
 * check the number of tasks waiting to be submitted to the QDMI device.
 * 
 */
int Submitter::getQueueSize() {
  return sub_queue.size();
}

/**
 * @brief Function submitting a task to the QDMI device
 *
 * This method will submit the task to the QDMI device using the QDMI API. It
 * will create a QDMI fragment from the task and submit it to the device.
 * 
 * @param task the task to submit
 * @return the status of the submission
 */
QDMI_Job_Status Submitter::submitTask(std::shared_ptr<QuantumTask> task, qdmi::Device device) {
  
  qdmi::Job job = device.create_job(
    QDMI_Program_Format::QDMI_PROGRAM_FORMAT_QASM2,
    task->CircuitFile,
    task->NumberShots);
  job.submit();
  job.wait();
  QDMI_Job_Status status = job.status();

  return status;
}

/**
 * @brief Function adding an observer to the Submitter
 *
 * Once added, the observer will be notified when a task is popped from the
 * queue. The observer should implement the onTaskPopped method to handle
 * the task.
 * 
 * @param observer an observer object to be added 
 */
void Submitter::addObserver(std::shared_ptr<ISubmitterObserver> observer) {
  observers.push_back(observer);
}

/**
 * @brief Function removing an observer
 *
 * Once removed, the observer will no longer be notified when a task is popped
 * from the queue.
 * 
 * @param observer an observer object to be removed 
 */
void Submitter::removeObserver(std::shared_ptr<ISubmitterObserver> observer) {
  observers.erase(std::remove(observers.begin(), observers.end(), observer),
                  observers.end());
}

#endif // QUANTUMTASK_CPP