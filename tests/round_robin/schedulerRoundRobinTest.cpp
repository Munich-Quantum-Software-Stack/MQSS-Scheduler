#include "round_robin.hpp"
#include "setup.hpp"
#include <iostream>
#include <memory>
#include <ostream>
#include <qdmi/client.h>
#include <quantum_task.hpp>
#include <submitter.hpp>
#include <unistd.h>
#include <unordered_map>
#include <vector>

int main() {
  std::cout << "--------------------------------------------\n";
  std::cout << "Testing MQSS Scheduler: Round Robin\n";
  std::cout << "--------------------------------------------\n";
  std::cout << std::endl;

  std::vector<std::shared_ptr<SchedulerQueue>> queues;
  queues = prepareQueues();
  std::cout << "1. Create SchedulerQueue: passed\n";

  // Map the devices to the Submitters
  std::unordered_map<QDMI_Device, std::shared_ptr<Submitter>> device2Submitter;
  std::vector<QDMI_Device> availableDevices = {};

  int count = 0;
  for (auto &queue: queues) {
    std::shared_ptr<Submitter> submitter = queue->mpSubmitter;
    QDMI_Device device = submitter->mDevice;
    // QDMI_Device_Property prop = 0;
    // char *name = (char *)malloc(256);
    // int result = QDMI_query_device_property_c(device, prop, &name);
    // free(name);

    device2Submitter[device] = submitter;
    availableDevices.push_back(device);
    count += 1;
  }
  std::cout << "2. Create submitter object: passed, count=" << count << std::endl;

  // Create some number of random tasks
  int numOfTasks = 10;
  std::vector<std::shared_ptr<QuantumTask>> tasks;

  for (int id = 0; id < numOfTasks; id++) {
    std::shared_ptr<QuantumTask> task = createRandomTask(id, availableDevices);
    tasks.push_back(task);
  }
  std::cout << "3. Create random tasks: passed, num=" << numOfTasks << std::endl;

  // Here the magic happens, the tasks are scheduled
  scheduler(queues, tasks);
  std::cout << "4. Schedule tasks: passed" << std::endl;

  // Iterate over all scheduled tasks and send them to the submitter
  for (auto task: tasks) {
    QDMI_Device scheduledDevice = task->mScheduledQpu;
    // QDMI_Device_Property prop = 0;
    char *name = (char *)malloc(256);

    // Send the task to corresponding submitter
    device2Submitter.at(scheduledDevice)->enqueue(task);

    // Retrieve and print the device name
    // int result = QDMI_query_device_property_c(scheduledDevice, prop, &name);
    // free(name);
  }

  // Wait for all tasks to be executed
  return allFinished(queues);
}