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
 * preferred devices to the tasks, and then schedules them using the Scheduler.
 */
#include "iostream"
#include "qdmi.h"
#include "queue.hpp"
#include "scheduler.hpp"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include <QuantumTask.hpp>
#include <Submitter.hpp>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <ostream>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace llvm;
using namespace llvm::orc;

#define CHECK_ERR(a, b)                                                        \
  {                                                                            \
    if (a != QDMI_SUCCESS) {                                                   \
      std::cout << std::endl << "[Error]: " << a << " at " << b;               \
    }                                                                          \
  }

/*
 * @brief Create a QuantumTask with a given taskID, some prefered devices and
 * its optional parentTask.
 */
std::shared_ptr<QuantumTask>
createTask(int taskID, const std::vector<QDMI_Device> &devices,
           std::shared_ptr<QuantumTask> parentTask = nullptr) {
  auto task = std::make_shared<QuantumTask>(taskID);

  std::filesystem::path currentFilePath = __FILE__; // Get the current file path
  std::filesystem::path currentDir =
      currentFilePath.parent_path(); // Get the directory of the current file
  std::filesystem::path inputFilePath =
      currentDir / ("inputs/example" + std::to_string(taskID % 3) +
                    ".ll"); // Construct the relative path

  // Create a new context and module for each task
  SMDiagnostic error;
  ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
  std::unique_ptr<Module> module =
      parseIRFile(inputFilePath.string(), error, *(TSCtx.getContext()));
  ThreadSafeModule TSM = ThreadSafeModule(std::move(module), std::move(TSCtx));

  // Initialize task
  task->mThreadSafeModule = std::move(TSM);
  task->pParentTask = parentTask; // Assign parent task
  // Add some preferred Devices in random order
  for (int k = 0; k < devices.size(); ++k) {
    task->mPreferredQpus.push_back(
        devices.at((taskID + k) % devices.size())); // Cycle through devices
  }
  task->mPriority =
      parentTask ? parentTask->mPriority : taskID % 3; // Set priority
  task->mNumberShots = 100;                            // Set number of shots

  return task;
}

int main() {
  std::cout << "   [Test]................Starting Scheduler Test." << std::endl;

  QInfo info; // We need this stuff to set up the devices
  QDMI_Session session = NULL;
  int err = QInfo_create(&info);
  CHECK_ERR(err, "QInfo_create");

  err = QDMI_session_init(info, &session);
  int count = -1;

  // Get the number of devices
  QDMI_core_device_count(&session, &count);
  std::cout << "   [Test]................Number of available devices: " << count
            << std::endl;

  std::unordered_map<QDMI_Device, std::shared_ptr<Submitter>> device2Submitter;
  std::vector<std::shared_ptr<SchedulerQueue>> queues;
  std::vector<QDMI_Device> devices = {};

  // Add one SchedulerQueue and Submitter for each device
  for (int index = 0; index < count; index++) {
    QDMI_Device device;
    err = QDMI_core_open_device(&session, index, &info, &device);
    CHECK_ERR(err, "QDMI_core_open_device");
    devices.push_back(device);

    // Init and store Submitter in map for later use
    auto submitter = std::make_shared<Submitter>(device, 3);
    device2Submitter.emplace(device, submitter);
    // Init and store SchedulerQueue in vector for later use
    auto schedulerQueue = std::make_shared<SchedulerQueue>(submitter);
    // Init observer pattern (must be done after initialization)
    schedulerQueue->initialize();
    queues.push_back(schedulerQueue);
  }

  std::cout << "   [Test]................Created Scheduler(Queue) and "
               "Submitter for each of the following devices:"
            << std::endl;
  for (auto &device : devices) {
    QDMI_Device_property prop;
    int name = -1;
    int result = QDMI_query_device_property_i(device, prop, &name);
    std::cout << "   [Test]................Device " << name << std::endl;
  }

  std::cout
      << "   [Test]................Simulating task generation and scheduling."
      << std::endl;

  // Simulate Generator by creating a bunch of tasks with possible child tasks
  std::vector<int> numChildTasks = {3, 2, 2, 5, 0, 3, 2};
  int numParentTasks = numChildTasks.size();
  int taskID = 0; // Unique task ID for each task

  std::cout << "   [Test]................Creating " << numParentTasks
            << " parent tasks with some child tasks." << std::endl;
  for (int i = 0; i < numParentTasks; ++i) {
    // Collection of to-be-scheduled tasks
    std::vector<std::shared_ptr<QuantumTask>> tasks;

    auto parentTask = createTask(taskID++, devices);
    if (numChildTasks[i] == 0) {
      tasks.push_back(parentTask);
      // Here the magic happens, the parent task is scheduled
      scheduler(queues, {parentTask});
      std::cout << "   [Test]................The following parent task without "
                   "child tasks has been scheduled: "
                << std::endl;
      // Get name of the device
      QDMI_Device_property prop;
      int name = -1;
      int result =
          QDMI_query_device_property_i(parentTask->mScheduledQpu, prop, &name);
      std::cout << "   [Test]................Task " << parentTask->mTaskId
                << " to Device " << name << ", with priority "
                << parentTask->mPriority << " and execution order  "
                << parentTask->mExecutionOrder << std::endl;
    } else {
      std::vector<std::shared_ptr<QuantumTask>> lastChildTasks = {};
      for (int j = 0; j < numChildTasks[i]; ++j) {
        auto childTask = createTask(taskID++, devices, parentTask);
        tasks.push_back(childTask);
        lastChildTasks.push_back(childTask);
      }
      // Here the magic happens, the child tasks are scheduled
      scheduler(queues, lastChildTasks);
      std::cout << "   [Test]................The following tasks have been "
                   "scheduled: "
                << std::endl;
      for (auto task : lastChildTasks) {
        // Get name of the device
        QDMI_Device_property prop;
        int name = -1;
        int result =
            QDMI_query_device_property_i(task->mScheduledQpu, prop, &name);

        std::cout << "   [Test]................Task " << task->mTaskId
                  << " to Device " << name << ", with priority "
                  << task->mPriority << " and execution order  "
                  << task->mExecutionOrder << std::endl;
      }
      lastChildTasks.clear();
    }

    // At this point the tasks would be further obtimized to the respective
    // devices
    std::cout << "   [Test]................After scheduling all tasks the "
                 "circuits are now optimized."
              << std::endl;
    std::cout << "   [Test]................Tasks then arrive in arbitrary "
                 "order at the respective Submitters."
              << std::endl;
    // Iterate over all scheduled tasks and send them to the respective
    // Submitter in random order
    for (auto task : tasks) {
      std::shared_ptr<Submitter> submitter =
          device2Submitter.at(task->mScheduledQpu);
      submitter->enqueue(task);
      std::cout << "   [Test]................Task " << task->mTaskId
                << " was sent to Submitter " << task->mScheduledQpu
                << std::endl;
    }
  }

  // Wait for all tasks to be executed (removed from the queue)
  std::cout << "   [Test]................Waiting for all tasks to be executed."
            << std::endl;
  while (true) {
    bool allFinished = true;
    for (auto queue : queues) {
      if (!queue->mTasks.empty()) {
        allFinished = false;
        break;
      }
    }
    if (allFinished) {
      break;
    }
    usleep(1000);
  }
  std::cout << "   [Test]................All tasks have been executed."
            << std::endl;
  return 0;
}