#include "heuristic.hpp"
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

using namespace llvm;
using namespace llvm::orc;

int main(){
    
    QInfo info;
    QDMI_Session session = NULL;
    int err = QInfo_create(&info);

    err = QDMI_session_init(info, &session);

    // TODO: enable once FOMAC works
    // std::vector<QDMI_Device> devices = FOMAC_available_devices(true);
    std::vector<My_QDMI_Device> devices = {My_QDMI_Device("q5")};//, My_QDMI_Device("q20"), My_QDMI_Device("wmi")};
    
    Submitter2Device device2Submitter;
    Scheduler2Device device2SchedQueue;

    for (const My_QDMI_Device& device : devices) {
        auto submitter = std::make_shared<Submitter>(device); // Create Submitter using smart pointer
        device2Submitter.emplace(device, submitter); // Store Submitter in map using smart pointer

        auto scheduler = std::make_shared<SchedulerQueue>(submitter);
        submitter->addObserver(scheduler); // Add the SchedulerQueue as an observer
        device2SchedQueue.emplace(device, scheduler); 
    }

    // Simulate Generator by creating a bunch of tasks
    std::vector<int> numChildTasks = {5, 5, 5};//, 2, 3, 0, 3, 1}; 
    int numParentTasks = numChildTasks.size();
    // Collection of to-be-scheduled tasks
    std::vector<std::shared_ptr<QuantumTask>> tasks;
    int taskID = 0; // Unique task ID for each task

    for (int i = 0; i < numParentTasks; ++i) {
        // Create a new QuantumTask object for each parent task
        auto parentTask = std::make_shared<QuantumTask>(taskID++);
        if (numChildTasks[i] == 0) {
            // Simulate Generator could not cut the circuit
            SMDiagnostic error;
            ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
            std::unique_ptr<Module> module = parseIRFile("/home/ubuntu/mqss/scheduler/inputs/example" + std::to_string(i%3) + ".ll", error, *(TSCtx.getContext()));
            ThreadSafeModule TSM = ThreadSafeModule(std::move(module),std::move(TSCtx));
            parentTask->mThreadSafeModule =  std::move(TSM);
            // Add some preferred QPUs
            for (int j = 0; j < devices.size(); ++j) {
                parentTask->mPreferredQpus.push_back(devices.at((i + j) % devices.size())); // Cycle through devices
            }
            parentTask->mPriority = i % 3; // Cycle through priorities
            parentTask->mNumberShots = 100; // Set number of shots

            tasks.push_back(parentTask);
            scheduler(device2SchedQueue, {parentTask});
        } else {
            // Simulate Generator cut the circuit into child tasks
            std::vector<std::shared_ptr<QuantumTask>> lastChildTasks = {};

            for (int j = 0; j < numChildTasks[i]; ++j) {
                // Create a new QuantumTask object for each child task
                auto childTask = std::make_shared<QuantumTask>(taskID++);

                // Create a new context and module for each task
                SMDiagnostic error;
                ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
                std::unique_ptr<Module> module = parseIRFile("/home/ubuntu/mqss/scheduler/inputs/example" + std::to_string(taskID%3) + ".ll", error, *(TSCtx.getContext()));
                ThreadSafeModule TSM = ThreadSafeModule(std::move(module),std::move(TSCtx));

                // Initialize task
                childTask->mThreadSafeModule =  std::move(TSM);
                childTask->pParentTask = parentTask; // Assign parent task
                // Add some preferred QPUs
                for (int k = 0; k < devices.size(); ++k) {
                    childTask->mPreferredQpus.push_back(devices.at((taskID + k) % devices.size())); // Cycle through devices
                }
                childTask->mPriority = taskID % 3; // Cycle through priorities
                parentTask->mNumberShots = 100; // Set number of shots

                // Add the child task to the to-be-scheduled tasks vector
                tasks.push_back(childTask);
                lastChildTasks.push_back(childTask);
            }            
            // Schedule all child tasks of one common parent together
            scheduler(device2SchedQueue, lastChildTasks);
            lastChildTasks.clear();
        }
    }

    // At this point the tasks would be further obtimized to the respective devices

    // Iterate over all scheduled tasks and send them to the respective Submitter
    for (auto task: tasks){
        std::shared_ptr<Submitter> submitter = device2Submitter.at(task->mScheduledQpu);
        submitter->insertTask(task);
    }

    // Wait for all tasks to be executed (removed from the queue)
    while (true) {
        bool allFinished = true;
        for (auto& [device, scheduler] : device2SchedQueue) {
            if (!scheduler->mTasks.empty()) {
                allFinished = false;
                break;
            }
        }
        if (allFinished) {
            break;
        }
        usleep(1000);
    } 
}