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
#include "qinfo/qinfo.h"
#include "qdmi/client.h"
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
 * @param preferredDevices The preferred device(s) for the task
 * @param parentTask The parent task of the task (optional, else nullptr)
 * @param priority The priority of the task (optional, else random)
 * @param shots The number of shots for the task (optional, else random)
 * @param duration The duration of the task (optional, else random)
 *
 * @return A shared pointer to the created QuantumTask
 */
std::shared_ptr<QuantumTask>
createRandomTask(int taskID, const std::vector<QDMI_Device> &preferredDevices,
                 std::shared_ptr<QuantumTask> parentTask, int priority,
                 double duration, int shots) {
  auto task = std::make_shared<QuantumTask>(taskID);

  // Input circuit file
  std::filesystem::path currentFilePath = __FILE__; // Get the current file path
  std::filesystem::path dirPath =
      currentFilePath.parent_path(); // Get the directory
  std::filesystem::path inputFilePath =
      dirPath / ("circuits/sample_circuit_" + std::to_string(taskID % 3) +
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

  // Add the preferred device(s)
  if (!preferredDevices.empty()) {
    task->mPreferredQpus = preferredDevices;
  }

  // Set random values if not provided (i.e. -1)
  if (priority == -1) {
    // Random priority between 0 and 2
    int rndPriority = rand() % 3;
    task->mPriority = parentTask ? parentTask->mPriority : rndPriority;
  } else {
    task->mPriority = priority;
  }
  if (duration == -1) {
    // Random duration between 0 and 9
    double rndDuration = rand() % 10;
    task->mDuration = rndDuration;
  } else {
    task->mDuration = duration;
  }
  if (shots == -1) {
    // Random number of shots between 1k and 10k
    int rndShots = (rand() % 10 + 1) * 1000;
    task->mNumberShots = rndShots;
  } else {
    task->mNumberShots = shots;
  }

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
  std::cout << "--------------------------------------------\n";
  std::cout << "[setup] preparing SchedulerQueues\n";

  QInfo info;
  QDMI_Session session = NULL;
  int err = QInfo_create(&info);
  if (QInfo_is_Error(err)) {
    std::cerr << "[setup] Warning: error by QInfo_create()\n";
  }
  // err = QDMI_session_init(info, &session);
  // CHECK_ERR(err, "[setup] QDMI_session_init\n");
  int count = -1;

  // Get the number of devices
  // QDMI_core_device_count(&session, &count);
  // std::cout << "[setup] Num. available devices: " << count << std::endl;

  std::unordered_map<QDMI_Device, std::shared_ptr<Submitter>> device2Submitter;
  std::vector<std::shared_ptr<SchedulerQueue>> queues;
  std::vector<QDMI_Device> devices = {};

  // Add one SchedulerQueue and Submitter for each device
  for (int index = 0; index < count; index++) {
    QDMI_Device device;
    // err = QDMI_core_open_device(&session, index, &info, &device);
    // CHECK_ERR(err, "[setup] QDMI_core_open_device\n");
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
  std::cout << "--------------------------------------------\n";

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
  std::cout << "[setup] Waiting for all tasks to be executed."
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
  std::cout << "[setup] All tasks have been executed."
            << std::endl;
  return 1;
}