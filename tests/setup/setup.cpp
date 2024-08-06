/*
 * @file setup.cpp
 * @brief This file provides helper functions for the scheduler tests.
 *
 * It is assumed, there will be one SchedulerQueue and one corresponding
 * Submitter per device. The test simulates the quantum resource manager (QRM)
 * by creating a set of devices and associated Submitters.
 * It additionally provides helper functions to create random tasks and to check
 * the queues when all tasks have been executed.
 */
#include "setup.hpp"
#include <filesystem>
#include <iostream>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <unistd.h>

using namespace llvm;
using namespace llvm::orc;

/*
 * @brief Create a QuantumTask
 *
 * @param taskID The ID of the task
 * @param availableDevices Devices to choose radom preferredQpus from
 * @param parentTask The optional parent task (default is nullptr)
 *
 * @return A shared pointer to the created QuantumTask
 */
std::shared_ptr<QuantumTask>
createRandomTask(int taskID, const std::vector<QDMI_Device> &availableDevices,
                 std::shared_ptr<QuantumTask> parentTask) {
  auto task = std::make_shared<QuantumTask>(taskID);

  // Input circuit file
  std::filesystem::path currentFilePath = __FILE__; // Get the current file path
  std::filesystem::path dirPath =
      currentFilePath.parent_path(); // Get the directory
  std::filesystem::path inputFilePath =
      dirPath / ("inputs/example" + std::to_string(taskID % 3) +
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

  // Add some preferred devices in random order
  for (int k = 0; k < availableDevices.size(); ++k) {
    task->mPreferredQpus.push_back(availableDevices.at(
        (taskID + k) % availableDevices.size())); // Cycle through devices
  }
  task->mPriority =
      parentTask ? parentTask->mPriority : taskID % 3; // Random priority
  task->mNumberShots = (100 * taskID) % 1000;          // Random number of shots

  return task;
}

/*
 * @brief Prepare the queues for the test
 *
 * This function gathers the available devices, associated submitters and
 * creates the corresponding SchedulerQueues.
 *
 * @return A vector of shared pointers to the SchedulerQueues
 */
std::vector<std::shared_ptr<SchedulerQueue>> prepareQueues() {
  std::cout << "   [Test]................Preparing SchedulerQueues."
            << std::endl;

  QInfo info; // We need this stuff to set up the devices
  QDMI_Session session = NULL;
  int err = QInfo_create(&info);
  if (QInfo_is_Error(err)) {
    std::cerr << "Warning: Error during QInfo_create" << std::endl;
  }
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

  return queues;
}

/*
 * @brief Check if all tasks have been executed
 *
 * @param queues A vector of shared pointers to the SchedulerQueues to check
 *
 * @return 1 when all tasks have been executed
 */
int allFinished(std::vector<std::shared_ptr<SchedulerQueue>> queues) {
  std::cout << "   [Test]................Waiting for all tasks to be executed."
            << std::endl;
  while (true) {
    bool allFinished = true;
    for (auto &queue : queues) {
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
  return 1;
}