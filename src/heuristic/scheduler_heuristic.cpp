/**
 * @file scheduler.cpp
 * @brief Implementation of a ML guided scheduler.
 */
#include "Submitter.hpp"
#include "predictor.hpp"
#include "queue.hpp"
#include <heuristic.hpp>
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
std::unordered_map<My_QDMI_Device, float> calculate_scores(QuantumTask *task)
{
    std::unordered_map<My_QDMI_Device, float> scores;

    // If the user only wants to use a single QPU
    if (task->mPreferredQpus.size() == 1)
    {
        // Assign maximum score to the only QPU
        scores = {{task->mPreferredQpus.front(), 1.0}};
    }
    // If the user wants to use some QPUs more than others
    // TODO: Implement this case
    // else if (user wants to use some QPUs more than others):
    //      scores = task.preferred_qpus
    else
    {
        // If choice is not forced or the user preference is equally distributed
        // TODO: Once My_QDMI_Device can be ID'd by a string, use it to select
        // the model
        std::vector<std::string> models;
        for (const auto &qpu : task->mPreferredQpus)
        {
            models.push_back(qpu.mName);
        }

        // Predict figure of merit for every device
        std::map<std::string, float> predictions =
            predict(task->mThreadSafeModule, models);

        for (const auto &device : task->mPreferredQpus)
        {
            scores[device] = predictions[device.mName];
        }
    }

    // Print the scores for debugging
    std::cout << "   [Scheduler]...........Scores: ";
    for (const auto &score : scores)
    {
        std::cout << score.first.mName << ": " << score.second << std::endl;
    }

    return scores;
}

/**
 * @brief Select the shortest queue among the top 3 scored devices.
 * @param task The QuantumTask to be scheduled.
 * @param scores The scores of the devices.
 * @return The name of the selected device.
 **/
My_QDMI_Device
choose_device(QuantumTask *task,
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

    // Print the names of the top 3 devices
    std::cout << std::endl
              << "   [Scheduler]...........Choosing target My_QDMI_Device from"
              << " the following devices: ";
    for (auto &device : devices)
    {
        std::cout << device.mName << " ";
    }

    // Find the queue with shortest end time among the three devices
    float min_end_time = std::numeric_limits<float>::max();
    My_QDMI_Device target_device = devices.front();

    for (auto &device : devices)
    {
        // Get the total duration of the queue for the current device
        double queueDuration = device2SchedQueue[device]->mTotalDuration;
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
int skipping_schedule(QuantumTask *pNewTask, SchedulerQueue *pQueue)
{
    // Extract the duration and priority of the new task
    float new_task_duration = pNewTask->mDuration;
    int new_task_priority = pNewTask->mPriority;

    // Define the increment for the age of a task when it is skipped
    float age_increment = 0.5;

    int i = 0;

    // If the queue is not empty, try to find a position for the new task
    if (pQueue->mTasks.size() != 0)
    {
        // Iterate over the tasks in the queue in reverse order
        for (i = pQueue->mTasks.size() - 1; i >= 0; --i)
        {
            QuantumTask *last_task = pQueue->mTasks[i];
            float last_task_priority = last_task->mPriority;

            // Predict the end time of the new task if it is inserted after the
            // current task
            float predicted_end = last_task->mEnd + new_task_duration;

            // If the new task has a higher priority, it can skip the current
            // task
            if (new_task_priority >
                std::floor(last_task_priority + last_task->mAge))
            {
                // Update the end time of the current task if necessary
                if (predicted_end > last_task->mEnd)
                {
                    last_task->mEnd = predicted_end;
                }

                // Increase the age of the current (skipped) task
                last_task->mAge += age_increment;

                // Continue with the next task in the queue
                continue;
            }
            // If the new task has the same priority, it can sometimes skip the
            // current task
            else if (new_task_priority == last_task_priority)
            {
                // Determine the parent task of the current task
                QuantumTask *parent_task = last_task->pParentTask != NULL
                                               ? last_task->pParentTask
                                               : last_task;
                float last_parent_end = parent_task->mEnd;

                // If the new task can be inserted without delaying the parent
                // task, it can skip the current task
                if (predicted_end < last_parent_end)
                {
                    // Determine the parent task of the new task
                    QuantumTask *new_parent_task = pNewTask->pParentTask != NULL
                                                       ? pNewTask->pParentTask
                                                       : pNewTask;

                    // If the new task can be inserted without delaying its
                    // parent task, it should skip the current task
                    if (new_parent_task->mEnd < last_parent_end)
                    {
                        // Update the end time of the current task if necessary
                        if (predicted_end > last_task->mEnd)
                        {
                            last_task->mEnd = predicted_end;
                        }

                        // Increase the age of the current (skipped) task
                        last_task->mAge += age_increment;

                        // Continue with the next task in the queue
                        continue;
                    }
                }
            }
            // If the new task cannot skip the current task, stop the search
            break;
        }
    }

    // Insert the new task at the found position in the queue
    pQueue->addTask(pNewTask, i);

    // Update the end time of the new task if necessary
    if (new_task_duration > pNewTask->mEnd)
    {
        pNewTask->mEnd = new_task_duration;
    }

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
                         std::vector<QuantumTask *> tasks)
{
    // Print the number of available devices
    std::cout << "   [Scheduler]..........." << device2SchedQueue.size()
              << " available device(s)" << std::endl;

    // Sort tasks by priority and within that by duration
    std::sort(tasks.begin(), tasks.end(),
              [](const QuantumTask *a, const QuantumTask *b)
              {
                  if (a->mPriority == b->mPriority)
                  {
                      // If priorities are equal, sort by duration
                      return a->mDuration > b->mDuration;
                  }
                  // Otherwise, sort by priority
                  return a->mPriority > b->mPriority;
              });

    // Process each task in the sorted list
    for (QuantumTask *task : tasks)
    {
        // Calculate scores for each (preferred) device based on the task
        std::unordered_map<My_QDMI_Device, float> scores =
            calculate_scores(task);

        // Choose the device with the shortest queue out of top 3 scored devices
        My_QDMI_Device target_device =
            choose_device(task, scores, device2SchedQueue);

        std::cout << "   [Scheduler]...........Inserting QuantumTask with ID "
                  << task->mTaskId << " into the queue for device "
                  << target_device.mName << std::endl;

        // Schedule the task on the chosen device and get the position where it
        // was inserted
        int position =
            skipping_schedule(task, device2SchedQueue[target_device].get());

        // Print the position where the task was inserted
        std::cout
            << "   [Scheduler]...........QuantumTask inserted at position "
            << position << std::endl;
    }

    return 0;
}
