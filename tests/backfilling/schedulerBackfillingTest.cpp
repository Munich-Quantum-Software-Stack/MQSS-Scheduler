/*
 * @file schedulerBackfillingTest.cpp
 * @brief This file simulates the scheduling of QuantumTasks using the
 * backfilling Scheduler.
 *
 * The main purpose of this test is to verify that the Scheduler can correctly
 * handle and schedule QuantumTasks, including tasks with parent-child
 * relationships and tasks with specific device preferences. This includes the
 * interplay between the Submitter instances and SchedulerQueues. It is assumed,
 * there will be one SchedulerQueue and one corresponding Submitter per device.
 * The test simulates the quantum resource manager (QRM) by creating a set of
 * devices and associated Submitters. It launches tasks, assignes priorities and
 * preferred devices to the tasks, and then schedules them with backfilling.
 */
#include "../setup/setup.hpp"
#include "backfilling.hpp"
#include <iostream>
#include <memory>
#include <ostream>
#include <qdmi.h>
#include <quantum_task.hpp>
#include <submitter.hpp>
#include <unistd.h>
#include <unordered_map>
#include <vector>

int main() {
  std::cout << "   [Test]................Starting Backfilling Scheduler test."
            << std::endl;

  std::vector<std::shared_ptr<SchedulerQueue>> queues;
  queues = prepareQueues(); // Get available SchedulerQueues

  std::cout << "   [Test]................Created Scheduler(Queue) and "
            << "Submitter for each of the following devices:" << std::endl;

  // Map the devices to the Submitters
  std::unordered_map<QDMI_Device, std::shared_ptr<Submitter>> device2Submitter;
  std::vector<QDMI_Device> availableDevices = {};

  for (auto &queue : queues) {
    std::shared_ptr<Submitter> submitter = queue->mpSubmitter;
    QDMI_Device device = submitter->mDevice;
    QDMI_Device_property prop;
    int name = -1;

    // Retrieve and print the device name
    int result = QDMI_query_device_property_i(device, prop, &name);
    std::cout << "   [Test]................Device " << name << std::endl;

    device2Submitter[device] = submitter; // For easy access later
    availableDevices.push_back(device);
  }

  // Simulate Generator by creating a bunch of tasks with possible child tasks
  std::vector<int> numChildTasks = {3, 2, 2, 5, 0, 3, 2};
  int numParentTasks = numChildTasks.size();
  int taskID = 0; // Unique task ID for each task

  std::cout << "   [Test]................Creating " << numParentTasks
            << " parent tasks with some child tasks." << std::endl;

  // Collection of all to-be-scheduled tasks
  std::vector<std::shared_ptr<QuantumTask>> tasks;

  for (int i = 0; i < numParentTasks; ++i) {

    // Create a parent task
    auto parentTask = createRandomTask(taskID++, availableDevices);

    if (numChildTasks[i] == 0) {
      // If parent task has no child tasks, schedule only parent task
      scheduler(queues, {parentTask});

      // Collect all tasks
      tasks.push_back(parentTask);

    } else {
      // If parent task has child tasks, schedule all children
      std::vector<std::shared_ptr<QuantumTask>> childTasks = {};

      // Create child tasks
      for (int j = 0; j < numChildTasks[i]; ++j) {
        auto childTask =
            createRandomTask(taskID++, availableDevices, parentTask);

        childTasks.push_back(childTask); // To be scheduled now
        tasks.push_back(childTask);      // Collect all tasks
      }

      // Here the magic happens, the child tasks are scheduled
      scheduler(queues, childTasks);
      childTasks.clear();
    }
  }
  std::cout << "   [Test]................All tasks have been scheduled and "
            << "will be sent to the following devices:" << std::endl;

  // Iterate over all scheduled tasks and send them to the submitter
  for (auto task : tasks) {
    QDMI_Device scheduledDevice = task->mScheduledQpu;
    QDMI_Device_property prop;
    int name = -1;

    // Send the task to corresponding submitter
    device2Submitter.at(scheduledDevice)->enqueue(task);

    // Retrieve and print the device name
    int result = QDMI_query_device_property_i(scheduledDevice, prop, &name);
    std::cout << "   [Test]................Task " << task->mTaskId
              << " was sent to submitter for device " << name << std::endl;
  }

  // Wait for all tasks to be executed (removed from the queue)
  return allFinished(queues);
}