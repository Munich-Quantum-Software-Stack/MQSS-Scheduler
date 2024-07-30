/*
 * @file schedulerBackfillingTest.cpp
 * @brief This file simulates the scheduling of QuantumTasks using the backfilling Scheduler.
 *
 * The main purpose of this test is to verify that the Scheduler can correctly
 * handle and schedule QuantumTasks, including tasks with parent-child relationships
 * and tasks with specific QPU preferences.
 * This includes the interplay between the Submitters and SchedulerQueues. 
 * It is assumed, there will be one SchedulerQueue and one corresponding Submitter per device.
 * The test simulates the quantum resource manager by creating a set of devices and associated Submitters.
 * It launches tasks, assignes priorities and preferred QPUs to the tasks, and then schedules them using the Scheduler.
 */
#include "qdmi.h"
#include "scheduler.hpp"
#include "iostream"
#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <numeric>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include "llvm/Support/SourceMgr.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IRReader/IRReader.h"
#include "QuantumTask.hpp"
#include "Submitter.hpp"
#include "queue.hpp"
#include <filesystem>

using namespace llvm;
using namespace llvm::orc;

/*
* @brief Create a QuantumTask with a given taskID, some prefered devices and its optional parentTask.
*/
std::shared_ptr<QuantumTask> createTask(int taskID, const std::vector<QDMI_Device>& devices, std::shared_ptr<QuantumTask> parentTask = nullptr) {
    auto task = std::make_shared<QuantumTask>(taskID);

    std::filesystem::path currentFilePath = __FILE__; // Get the current file path
    std::filesystem::path currentDir = currentFilePath.parent_path(); // Get the directory of the current file
    std::filesystem::path inputFilePath = currentDir / ("inputs/example" + std::to_string(taskID % 3) + ".ll"); // Construct the relative path

    // Create a new context and module for each task
    SMDiagnostic error;
    ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
    std::unique_ptr<Module> module = parseIRFile(inputFilePath.string(), error, *(TSCtx.getContext()));
    ThreadSafeModule TSM = ThreadSafeModule(std::move(module), std::move(TSCtx));

    // Initialize task
    task->mThreadSafeModule =  std::move(TSM);
    task->pParentTask = parentTask; // Assign parent task
    // Add some preferred QPUs in random order
    for (int k = 0; k < devices.size(); ++k) {
        task->mPreferredQpus.push_back(devices.at((taskID + k) % devices.size())); // Cycle through devices
    }
    task->mPriority = parentTask ? parentTask->mPriority : taskID % 3; // Set priority
    task->mNumberShots = 100; // Set number of shots

    return task;
}

int main(){
    std::cout << "   [Test]................Starting Scheduler Backfilling Test" << std::endl;
    QInfo info; // We need this stuff to set up the devices
    QDMI_Session session = NULL;
    int err = QInfo_create(&info);
    err = QDMI_session_init(info, &session);

    int count = -1;
    std::cout << "   [Test]................Querying devices" << std::endl;
    err = QDMI_core_device_count(&session, &count);
    std::vector<QDMI_Device> devices;
    for(int i = 0; i < count; i++){
        QDMI_Device device;
        err = QDMI_core_open_device(&session, i, &info, &device);
        devices.push_back(device);
        // TODO: remove once there are more than one device available
        devices.push_back(device);
        devices.push_back(device);
        devices.push_back(device);
        devices.push_back(device);
    } 
    std::cout << "   [Test]................Found " << devices.size() << " devices" << std::endl;
    for (auto& device : devices) {
        QDMI_Device_property prop;
        int name = -1;
        int result = QDMI_query_device_property_i(device, prop, &name);
        std::cout << "   [Test]................Device " << name << "\n";
    }

    // Create a map of devices to submitters
    std::unordered_map<QDMI_Device, std::shared_ptr<Submitter>> device2Submitter;
    // There will be one SchedulerQueue per device (Submitter)
    std::vector<std::shared_ptr<SchedulerQueue>> queues;

    std::cout << "   [Test]................Creating SchedulerQueues and associated Submitters for each device." << std::endl;
    for (const QDMI_Device& device : devices) {
        auto submitter = std::make_shared<Submitter>(device, 3); // Create Submitter using smart pointer
        device2Submitter.emplace(device, submitter); // Store Submitter in map using smart pointer

        auto scheduler = std::make_shared<SchedulerQueue>(submitter);
        submitter->addObserver(scheduler); // Add the SchedulerQueue as an observer
        queues.push_back(scheduler); // Store the SchedulerQueues in a vector
    }

    std::cout << "   [Test]................Simulating task generation and scheduling." << std::endl;
    // Simulate Generator by creating a bunch of tasks with possible child tasks
    std::vector<int> numChildTasks = {3, 2, 2, 5, 0, 3, 2};
    int numParentTasks = numChildTasks.size();
    int taskID = 0; // Unique task ID for each task

    std::cout<< "   [Test]................Creating " << numParentTasks << " parent tasks with some child tasks." << std::endl;
    for (int i = 0; i < numParentTasks; ++i) {
        // Collection of to-be-scheduled tasks
        std::vector<std::shared_ptr<QuantumTask>> tasks;

        auto parentTask = createTask(taskID++, devices);
        if (numChildTasks[i] == 0) {
            tasks.push_back(parentTask);
            scheduler(queues, {parentTask});
            std::cout << "   [Test]................The following parent task without child tasks has been scheduled: " << std::endl;
            QDMI_Device_property prop;
            int name = -1;
            int result = QDMI_query_device_property_i( parentTask->mScheduledQpu, prop, &name);
            std::cout << "   [Test]................Task " << parentTask->mTaskId << " to QPU " << name << ", with priority " << parentTask->mPriority << " and execution order  " << parentTask->mExecutionOrder << std::endl;
        } else {
            std::vector<std::shared_ptr<QuantumTask>> lastChildTasks = {};
            for (int j = 0; j < numChildTasks[i]; ++j) {
                auto childTask = createTask(taskID++, devices, parentTask);
                tasks.push_back(childTask);
                lastChildTasks.push_back(childTask);
            }      
            scheduler(queues, lastChildTasks);
            std::cout << "   [Test]................The following tasks have been scheduled: " << std::endl;
            for (auto task : lastChildTasks) {
                std::cout << "   [Test]................Task " << task->mTaskId << " to QPU " << task->mScheduledQpu << ", with priority " << task->mPriority << " and execution order  " << task->mExecutionOrder << std::endl;
            }
            lastChildTasks.clear();
        }

        // At this point the tasks would be further obtimized to the respective devices
        std::cout << "   [Test]................After scheduling all tasks the circuits are now optimized." << std::endl;
        std::cout << "   [Test]................Tasks then arrive in arbitrary order at the respective Submitters." << std::endl;
        // Iterate over all scheduled tasks and send them to the respective Submitter in random order
        for (auto task : tasks) {
            std::shared_ptr<Submitter> submitter = device2Submitter.at(task->mScheduledQpu);
            submitter->enqueue(task);
            std::cout << "   [Test]................Task " << task->mTaskId << " has been enqueued to Submitter " << task->mScheduledQpu << std::endl;
        }
    }

    // Wait for all tasks to be executed (removed from the queue)
    std::cout << "   [Test]................Waiting for all tasks to be executed." << std::endl;
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
    std::cout << "   [Test]................All tasks have been executed." << std::endl;
    return 0;
}