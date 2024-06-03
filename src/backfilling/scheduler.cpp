/**
 * @file scheduler.cpp
 * @brief Implementation of a ML guided scheduler.
 */
#include "QuantumTask.hpp"
#include "Submitter.hpp"
#include "predictor.hpp"
#include "queue.hpp"
#include <scheduler.hpp>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

/**
 * @brief Calculate scores for the devices based on user preference, ML model,
 * or both.
 * @param task The QuantumTask to be scheduled.
 * @return A map of devices to their respective scores.
 */
std::unordered_map<My_QDMI_Device, float>
calculate_scores(std::shared_ptr<QuantumTask> task)
{
    std::unordered_map<My_QDMI_Device, float> deviceScores = {};
    // TODO: get available devices from FOMAC/QDMI
    // Note: these are only test models and not ready for deployment
    std::unordered_map<My_QDMI_Device, string> device2Model = {
        {My_QDMI_Device("q20"), "q5_ga_depth.onnx"},
        {My_QDMI_Device("q5"), "q20_ga_depth.onnx"},
        {My_QDMI_Device("wmi"), "wmi_ga_depth.onnx"}};
    std::vector<string> models;

    // If the user only wants to use a single QPU
    if (task->mPreferredQpus.size() == 1)
    {
        // Assign maximum score to the only QPU
        deviceScores = {{task->mPreferredQpus.front(), 1.0}};
        return deviceScores;
    }
    // If the user preference is not specified, use the ML model
    else if (task->mPreferredQpus.size() == 0)
    {
        for (const auto &d2M : device2Model)
        {
            models.push_back(d2M.second);
        }
    }
    // If choice is not forced or the user preference is equally distributed
    else
    {
        for (const auto &qpu : task->mPreferredQpus)
        {
            models.push_back(device2Model[qpu]);
        }
    }

    // Predict figure of merit (FOM) for every device
    std::map<std::string, float> predictions =
        predict(task->mThreadSafeModule, models);

    // Assign scores to the devices based on the predictions
    for (const auto &model : models)
    {
        // Get device associated with the model
        auto it = std::find_if(device2Model.begin(), device2Model.end(),
                               [&model](const auto &d2M)
                               { return d2M.second == model; });
        if (it != device2Model.end())
        {
            My_QDMI_Device device = it->first;
            // Example FOM is "depth" -> lower is better
            deviceScores[device] = 1 / predictions[model];
        }
    }
    return deviceScores;
}

/**
 * @brief Select the shortest queue among the top 3 scored devices.
 * @param task The QuantumTask to be scheduled.
 * @param scores The scores of the devices.
 * @return The name of the selected device.
 **/
My_QDMI_Device
choose_device(std::shared_ptr<QuantumTask> task,
              const std::unordered_map<My_QDMI_Device, float> &scores,
              Scheduler2Device device2SchedQueue)
{
    // Find the devices with the three highest final scores
    std::vector<My_QDMI_Device> devices;
    for (auto &score : scores)
    {
        // If we have less than 3 devices, just add the current device
        if (devices.size() < 3)
        {
            devices.push_back(score.first);
        }
        else
        {
            // If we already have 3 devices, replace the one with the lowest
            // score if the current device has a higher score
            for (auto &device : devices)
            {
                if (score.second > scores.at(device))
                {
                    device = score.first;
                    break;
                }
            }
        }
    }

    std::cout
        << "   [Scheduler]...........Choosing target from top 3 devices: ";
    for (auto &device : devices)
    {
        std::cout << device.mName << " ";
    }
    std::cout << std::endl;

    // Find the queue with shortest end time among the three devices
    float min_end_time = std::numeric_limits<float>::max();
    My_QDMI_Device target_device = devices.front();

    for (auto &device : devices)
    {
        // Get the total duration of the queue for the current device
        float queueDuration = device2SchedQueue[device]->mTotalDuration;
        // If this duration is less than the current minimum, update the minimum
        // and set the current device as the target device
        if (queueDuration < min_end_time)
        {
            min_end_time = queueDuration;
            target_device = device;
        }
    }
    // Return the device with the shortest queue
    return target_device;
}

/**
 * @brief Schedule a QuantumTask on a target device using skipping strategy.
 * @param pNewTask The QuantumTask to be scheduled.
 * @param pQueue The target device's queue to schedule the QuantumTask on.
 * @return The position where the new task was inserted in the queue.
 */
int skipping_schedule(std::shared_ptr<QuantumTask> pNewTask,
                      std::shared_ptr<SchedulerQueue> pQueue)
{
    // Extract the duration and priority of the new task
    float newTaskDuration = pNewTask->mDuration;
    int newTaskPriority = pNewTask->mPriority;
    std::shared_ptr<QuantumTask> newParentTask =
        pNewTask->pParentTask != NULL ? pNewTask->pParentTask : pNewTask;
    float newParentEnd = newParentTask->mEnd;

    // Define the increment for the age of a task when it is skipped
    float ageIncrement = 0.5;

    int i = 0;

    // If the queue is not empty, try to find a position for the new task
    if (pQueue->mTasks.size() != 0)
    {
        // Iterate over the tasks in the queue in reverse order
        for (i = pQueue->mTasks.size(); i > 0; --i)
        {
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
            if (newTaskPriority >
                std::floor(lastTaskPriority + lastTaskAge))
            {
                // Update the end time of the last (to-be-skipped) task
                lastTask->mEnd = updatedEnd;

                // Update the end time of the last parent task
                if (updatedEnd > lastParentEnd)
                {
                    lastParentTask->mEnd = updatedEnd;
                }
                // Increase the age of the last (to-be-skipped) task
                lastTask->mAge += ageIncrement;

                // Continue with the next task in the queue
                continue;
            }
            // Possibly skip if the new task has the same priority
            else if (newTaskPriority == lastTaskPriority)
            {
                // Possibly skip if the new task can be inserted without delaying the last parent task
                if (updatedEnd < lastParentEnd)
                {
                    // Skip if we actually gain something
                    if (newParentEnd < lastParentEnd)
                    {
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
 * @param device2SchedQueue Mapping from devices to their respective scheduler
 * queues.
 * @param tasks Vector of tasks to be scheduled.
 * @return The selected device on which the task was scheduled.
 */
extern "C" int scheduler(Scheduler2Device device2SchedQueue,
                         std::vector<std::shared_ptr<QuantumTask>> tasks)
{   
    // Predict the expected duration for each (optimized) circuit
    // Note: this is only a test model and not ready for deployment
    for (std::shared_ptr<QuantumTask> task : tasks)
    {
        std::map<std::string, float> prediction = predict(task->mThreadSafeModule, {"ga_depth.onnx"});
        task->mDuration = prediction["ga_depth.onnx"] * task->mNumberShots;
    }

    // Sort tasks by priority and within that by duration
    std::sort(tasks.begin(), tasks.end(),
              [](const std::shared_ptr<QuantumTask> a,
                 const std::shared_ptr<QuantumTask> b)
              {
                  if (a->mPriority == b->mPriority)
                  {
                      // If priorities are equal, sort by duration
                      return a->mDuration > b->mDuration;
                  }
                  // Otherwise, sort by priority
                  return a->mPriority > b->mPriority;
              });

    for (int i = 0; i < tasks.size(); ++i)
    {
        std::cout << "   [Scheduler]..........."
                  << ": Task ID: " << tasks[i]->mTaskId
                  << ", Duration: " << tasks[i]->mDuration
                  << ", Priority: " << tasks[i]->mPriority << std::endl;
    }

    // Process each task in the sorted list
    for (std::shared_ptr<QuantumTask> task : tasks)
    {
        std::cout << "   [Scheduler]...........Processing QuantumTask with ID "
                  << task->mTaskId << std::endl;

        // Calculate scores for each (preferred) device based on the task
        std::unordered_map<My_QDMI_Device, float> scores =
            calculate_scores(task);

        std::cout << "   [Scheduler]...........Scores: ";
        for (const auto &score : scores)
        {
            std::cout << score.first.mName << ": " << score.second << " ";
        }
        std::cout << std::endl;

        // Choose the device with the shortest queue out of top 3 scored devices
        My_QDMI_Device target_device =
            choose_device(task, scores, device2SchedQueue);

        std::cout << "   [Scheduler]...........Inserting QuantumTask into the "
                     "queue for device "
                  << target_device.mName << std::endl << std::endl;
        task->mScheduledQpu = target_device;

        // Schedule the task on the chosen device and get the position where it
        // was inserted
        int position =
            skipping_schedule(task, device2SchedQueue[target_device]);
    }
    return 0;
}
