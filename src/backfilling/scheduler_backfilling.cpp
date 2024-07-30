/**
 * @file scheduler.cpp
 * @brief Implementation of a ML guided scheduler.
 */
#include "QuantumTask.hpp"
#include "Submitter.hpp"
#include "backfilling.hpp"
#include "predictor.hpp"
#include "qdmi.h"
#include "queue.hpp"
#include <iostream>
#include <memory>
#include <ostream>
#include <scheduler.hpp>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Calculate scores for the devices based on user preference, ML model,
 * or both. {preferredQPUs} ⋂ {availableQPUs} = {matchingQPUs} ≠ {} ->
 * predictScore(matchingQPUs) {preferredQPUs} ⋂ {availableQPUs} = {} ->
 * predictScore(availableQPUs) {availableQPUs} = {} -> ERROR
 * @param task The QuantumTask to be scheduled.
 * @return A map of devices to their respective scores.
 */
std::unordered_map<QDMI_Device, float>
calculate_scores(std::shared_ptr<QuantumTask> task) {
  std::unordered_map<QDMI_Device, float> deviceScores = {};

  // TODO: get count from QDMI_core_device_count(&session, &count);
  // Get the number of all available devices
  int count = task->mPreferredQpus.size();

  if (count == 0) {
    std::cerr << "   [Scheduler]...........No available devices found."
              << std::endl;
    return deviceScores;
  }

  // Associate each available device with a model
  std::unordered_map<QDMI_Device, std::string> availableQPUs2Model = {};
  std::unordered_map<std::string, QDMI_Device> model2availableQPUs = {};
  for (int i = 0; i < count; i++) {
    QDMI_Device device;
    // TODO: get device from QDMI_core_open_device(&session, i, &info, &device);
    device = task->mPreferredQpus.at(i);
    // TODO: use actual model names
    availableQPUs2Model[device] = "ga_depth.onnx";
    model2availableQPUs["ga_depth.onnx"] = device;
  }

  std::vector<string> chosenModels;
  // Select all models, from the user's preferred QPUs that are available
  for (const auto &qpu : task->mPreferredQpus) {
    if (availableQPUs2Model.find(qpu) == availableQPUs2Model.end()) {
      std::cerr << "   [Scheduler]...........Device not found in the list of "
                   "available devices."
                << std::endl;
      continue;
    } else {
      chosenModels.push_back(availableQPUs2Model[qpu]);
    }
  }

  // If none of the user preferences are available, score all available devices
  if (chosenModels.size() == 0) {
    for (const auto &d2M : availableQPUs2Model) {
      chosenModels.push_back(d2M.second);
    }
  }

  // Predict figure of merit (FOM) for all chosen models
  std::map<std::string, float> predictions =
      predict(task->mThreadSafeModule, chosenModels);

  // Assign scores to the devices based on the predictions
  for (const auto &model : chosenModels) {
    // Get device associated with the model
    QDMI_Device device = model2availableQPUs[model];
    // Example FOM is "depth" -> lower is better
    deviceScores[device] = 1 / predictions[model];
  }
  return deviceScores;
}

/**
 * @brief Select the shortest queue among the top 3 scored devices.
 * @param task The QuantumTask to be scheduled.
 * @param scores The scores of the devices.
 * @return The name of the selected device.
 **/
QDMI_Device
choose_device(std::shared_ptr<QuantumTask> task,
              const std::unordered_map<QDMI_Device, float> &scores,
              std::vector<std::shared_ptr<SchedulerQueue>> schedulerQueues) {
  // Find the devices with the three highest final scores
  std::vector<QDMI_Device> topDevices;
  for (auto &score : scores) {
    // If we have less than 3 devices, just add the current device
    if (topDevices.size() < 3) {
      topDevices.push_back(score.first);
    } else {
      // If we already have 3 devices, replace the one with the lowest
      // score if the current device has a higher score
      for (auto &device : topDevices) {
        if (score.second > scores.at(device)) {
          device = score.first;
          break;
        }
      }
    }
  }

  // Find the queue with shortest end time among the three devices
  float minEndTime = std::numeric_limits<float>::max();
  QDMI_Device targetDevice = topDevices.front();

  for (auto &device : topDevices) {
    // Get the associated queue for the selected device
    SchedulerQueue *schedulerQueue = nullptr;
    for (const auto &queue : schedulerQueues) {
      if (queue->mpSubmitter->mDevice == device) {
        schedulerQueue = queue.get();
        break;
      }
    }
    // Get the total duration of the queue for the current device
    float queueDuration = schedulerQueue->mTotalDuration;

    // If this duration is less than the current minimum, update the minimum
    // and set the current device as the target device
    if (queueDuration < minEndTime) {
      minEndTime = queueDuration;
      targetDevice = device;
    }
  }
  // Return the device with the shortest queue
  return targetDevice;
}

/**
 * @brief Schedule a QuantumTask on a target device using backfilling strategy.
 * @param pNewTask The QuantumTask to be scheduled.
 * @param pQueue The target device's queue to schedule the QuantumTask on.
 * @return The position where the new task was inserted in the queue.
 */
int backfilling(std::shared_ptr<QuantumTask> pNewTask,
                std::shared_ptr<SchedulerQueue> pQueue) {
  // Extract the duration and priority of the new task
  float newTaskDuration = pNewTask->mDuration;
  int newTaskPriority = pNewTask->mPriority;
  std::shared_ptr<QuantumTask> newParentTask =
      pNewTask->pParentTask != NULL ? pNewTask->pParentTask : pNewTask;
  float newParentEnd = newParentTask->mEnd;

  // Define the increment for the age of a task when it is skipped
  float ageIncrement = 0.25; // TODO: tune this parameter

  int i = 0;

  // If the queue is not empty, try to find a position for the new task
  if (pQueue->mTasks.size() != 0) {
    // Iterate over the tasks in the queue in reverse order
    for (i = pQueue->mTasks.size(); i > 0; --i) {
      // Get the last task in the queue at position i - 1
      std::shared_ptr<QuantumTask> lastTask = pQueue->mTasks[i - 1];
      float lastTaskPriority = lastTask->mPriority;
      float lastTaskEnd = lastTask->mEnd;
      float lastTaskAge = lastTask->mAge;

      // Determine the parent task of the last task
      std::shared_ptr<QuantumTask> lastParentTask =
          lastTask->pParentTask != NULL ? lastTask->pParentTask : lastTask;

      float lastParentEnd = lastParentTask->mEnd;

      // End time of the new task if it is inserted after the last task
      float updatedEnd = lastTaskEnd + newTaskDuration;

      // Skip if new task has a higher priority
      if (newTaskPriority > std::floor(lastTaskPriority + lastTaskAge)) {
        // Update the end time of the last (to-be-skipped) task
        lastTask->mEnd = updatedEnd;

        // Update the end time of the last parent task
        if (updatedEnd > lastParentEnd) {
          lastParentTask->mEnd = updatedEnd;
        }
        // Increase the age of the last (to-be-skipped) task
        lastTask->mAge += ageIncrement;

        // Continue with the next task in the queue
        continue;
      }
      // Possibly skip if the new task has the same priority
      else if (newTaskPriority == lastTaskPriority) {
        // Possibly skip if the new task can be inserted without delaying the
        // last parent task
        if (updatedEnd < lastParentEnd) {
          // Skip if we actually gain something
          if (newParentEnd < lastParentEnd) {
            // Update the end time of the last (to-be-skipped) task
            lastTask->mEnd = updatedEnd;

            // Increase the age of the last (to-be-skipped) task
            lastTask->mAge += ageIncrement;

            // Continue with the next task in the queue
            continue;
          }
        }
      }
      // Stop the search, if the new task cannot skip (any more)
      // Update the end time of the new task
      pNewTask->mEnd = updatedEnd;
      break;
    }
  } else {
    // Queue is empty
    // Update the end time of the new task
    pNewTask->mEnd = newTaskDuration;
  }

  // Update the end time of the parent task of the new task
  if (newParentEnd < pNewTask->mEnd) {
    newParentTask->mEnd = pNewTask->mEnd;
  }

  // Insert the new task at the found position in the queue
  pQueue->addTask(pNewTask, i);

  // Return the position where the new task was inserted
  return i;
}

/**
 * @brief Entry point for the scheduler.
 *
 * For each task in the list, the scheduler calculates the expected duration
 * (device independent) and sorts the tasks by priority and duration. Then, for
 * each task, the scheduler calculates scores for each device based on a trained
 * ML model and chooses the device with the shortest queue out of the top 3
 * scored devices. The task is then scheduled on the chosen device using a
 * backfilling strategy that tries to minimize the overall time the common
 * parent task takes to complete.
 *
 * @param schedulerQueues Vector of queues for each device.
 * @param tasks Vector of tasks to be scheduled.
 * @return 0 once all tasks have been scheduled.
 */
extern "C" int
scheduler(std::vector<std::shared_ptr<SchedulerQueue>> schedulerQueues,
          std::vector<std::shared_ptr<QuantumTask>> tasks) {
  // Get all available devices
  std::vector<QDMI_Device> availableDevices = {};
  for (const auto &queue : schedulerQueues) {
    availableDevices.push_back(queue->mpSubmitter->mDevice);
  }

  // Calculate expected duration for each circuit to sort tasks accordingly
  for (std::shared_ptr<QuantumTask> task : tasks) {
    // Since we dont know the target device yet, we use generic values
    // (resembling IQM gate times) This is only a guess to sort the tasks by
    // their size and will be updated later
    double singleQubitGateTime = 1.0;
    double twoQubitGateTime = 3 * singleQubitGateTime;
    double measurementTime = 350 * singleQubitGateTime;
    double duration =
        calculate_circuit_duration(task->mThreadSafeModule, singleQubitGateTime,
                                   twoQubitGateTime, measurementTime);
    task->mDuration = duration * task->mNumberShots;
  }

  // Sort tasks by priority and within that by duration
  std::sort(tasks.begin(), tasks.end(),
            [](const std::shared_ptr<QuantumTask> a,
               const std::shared_ptr<QuantumTask> b) {
              if (a->mPriority == b->mPriority) {
                // If priorities are equal, sort by duration
                return a->mDuration > b->mDuration;
              }
              // Otherwise, sort by priority
              return a->mPriority > b->mPriority;
            });

  // Process each task in the sorted list
  for (std::shared_ptr<QuantumTask> task : tasks) {
    // Calculate scores for each (preferred) device based on the task
    std::unordered_map<QDMI_Device, float> scores = calculate_scores(task);

    // Choose the device with the shortest queue out of top 3 scored devices
    QDMI_Device targetDevice = choose_device(task, scores, schedulerQueues);

    task->mScheduledQpu = targetDevice;

    // Update the expected duration of the task based on the chosen device
    std::map<std::string, float> prediction =
        predict(task->mThreadSafeModule, {"ga_depth.onnx"});
    task->mDuration = prediction["ga_depth.onnx"] * task->mNumberShots;

    // Get the corresponding queue for the selected device
    std::shared_ptr<SchedulerQueue> schedulerQueue = nullptr;
    for (const auto &queue : schedulerQueues) {
      if (queue->mpSubmitter->mDevice == targetDevice) {
        schedulerQueue = queue;
        break;
      }
    }
    // Schedule the task on the chosen device using backfilling strategy
    int position = backfilling(task, schedulerQueue);
  }
  return 0;
}
