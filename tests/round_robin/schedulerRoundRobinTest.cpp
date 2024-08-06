#include "../setup/setup.hpp"
#include "round_robin.hpp"
#include "scheduler.hpp"
#include <QuantumTask.hpp>
#include <Submitter.hpp>
#include <iostream>
#include <memory>
#include <ostream>
#include <qdmi.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

int main() {
  std::cout << "   [Test]................Starting Round-Robin Scheduler test."
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

  // Create some number of random tasks
  int numOfTasks = 10;
  std::vector<std::shared_ptr<QuantumTask>> tasks;

  for (int id = 0; id < numOfTasks; id++) {
    std::shared_ptr<QuantumTask> task = createRandomTask(id, availableDevices);
    tasks.push_back(task);
  }

  std::cout << "   [Test]................Scheduling " << numOfTasks
            << " QuantumTasks." << std::endl;

  // Here the magic happens, the tasks are scheduled
  scheduler(queues, tasks);

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
              << " was sent to Submitter " << name << std::endl;
  }

  // Wait for all tasks to be executed (removed from the queue)
  return allFinished(queues);
}