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
 *
 * Test scenario:
 * We want to schedule 3 (parent) tasks onto 2 devices.
 *
 * Task A has 2 child tasks:
 *   - device 0, priority 0, duration 4 |A A A A|
 *   - device 1, priority 0, duration 2 |A A|
 *
 * Task B has 2 child tasks:
 *   - device 0, priority 1, duration 1 |B|
 *   - device 1, priority 1, duration 2 |B B|
 *
 * Task C has 1 child task (i.e. ONLY parent task):
 *   - device 1, priority 0, duration 1 |C|
 *
 *
 * For successfull backfilling, the previously empty queues should look like
 * following (each letter = timestep, e.g. duration 2 = |X X|):
 *
 * Queue 0: |B|A A A A|
 * Queue 1: |B B|C C|A A|
 *
 */
#include "backfilling.hpp"
#include "setup.hpp"
#include <iostream>
#include <memory>
#include <ostream>
#include <qdmi.h>
#include <quantum_task.hpp>
#include <submitter.hpp>
#include <unistd.h>
#include <unordered_map>
#include <vector>

std::string get_device_name(QDMI_Device device) {
  QDMI_Device_property prop = BACKEND_NAME;
  char *name = (char *)malloc(256);
  int result = QDMI_query_device_property_c(device, prop, &name);
  std::string deviceName = name;
  free(name);
  return deviceName;
}

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
    QDMI_Device_property prop = BACKEND_NAME;
    char *name = (char *)malloc(256);

    // Retrieve and print the device name
    int result = QDMI_query_device_property_c(device, prop, &name);
    std::cout << "   [Test]................Device " << name << std::endl;
    free(name);

    device2Submitter[device] = submitter; // For easy access later
    availableDevices.push_back(device);
  }

  // Simulate Generator by creating a bunch of tasks with possible child tasks
  std::vector<int> parentPriorities = {0, 1, 0};
  std::vector<int> numChildTasks = {2, 2, 1};
  std::vector<std::vector<int>> durations = {{4, 2}, {1, 1}, {1}};
  std::vector<std::vector<int>> deviceIdx = {{0, 1}, {0, 1}, {1}};
  int numParentTasks = numChildTasks.size();
  int taskID = 0; // Unique task ID for each task

  std::cout << "   [Test]................Creating " << numParentTasks
            << " parent tasks with some child tasks." << std::endl;

  // Collection of all to-be-scheduled tasks
  std::vector<std::shared_ptr<QuantumTask>> tasks;

  for (int i = 0; i < numParentTasks; ++i) {

    // Create a parent task with preferred device, priority and duration
    auto parentTask =
        createRandomTask(taskID++, {availableDevices.at(deviceIdx[i][0])},
                         nullptr, parentPriorities[i], durations[i][0], 1);

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

        // Manually set preferred device, priority and duration
        auto childTask = createRandomTask(
            taskID++, {availableDevices.at(deviceIdx[i][j])}, parentTask,
            parentPriorities[i], durations[i][j], 1);

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

    // Send the task to corresponding submitter
    QDMI_Device scheduledDevice = task->mScheduledQpu;
    device2Submitter.at(scheduledDevice)->enqueue(task);

    // Retrieve and print the device name
    std::cout << "   [Test]................Task " << task->mTaskId
              << " was sent to submitter for device "
              << get_device_name(scheduledDevice) << " with execution order "
              << task->mExecutionOrder << " and preferred QPU "
              << get_device_name(task->mPreferredQpus[0]) << std::endl;
  }

  // Assert correct execution order
  // Queue 0: |B|A A A A|
  // Queue 1: |B B|C C|A A|
  auto AAAA = tasks[0];
  auto AA = tasks[1];
  auto B = tasks[2];
  auto BB = tasks[3];
  auto CC = tasks[4];

  // Assert correct devices
  assert(AAAA->mScheduledQpu == AAAA->mPreferredQpus[0]);
  assert(AA->mScheduledQpu == AA->mPreferredQpus[0]);
  assert(B->mScheduledQpu == B->mPreferredQpus[0]);
  assert(BB->mScheduledQpu == BB->mPreferredQpus[0]);
  assert(CC->mScheduledQpu == CC->mPreferredQpus[0]);

  // Assert correct execution order for Queue 0
  assert(AAAA->mScheduledQpu == B->mScheduledQpu);
  assert(B->mExecutionOrder < AAAA->mExecutionOrder);

  // Assert correct execution order for Queue 1
  assert(BB->mScheduledQpu == CC->mScheduledQpu &&
         CC->mScheduledQpu == AA->mScheduledQpu);
  assert(BB->mExecutionOrder < CC->mExecutionOrder &&
         CC->mExecutionOrder < AA->mExecutionOrder);

  // Wait for all tasks to be executed (removed from the queue)
  return allFinished(queues);
}