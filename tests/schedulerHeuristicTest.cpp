
#include "heuristic.hpp"
#include "iostream"
#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include "llvm/Support/SourceMgr.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IRReader/IRReader.h"

#include "Submitter.hpp"
#include "qdmi.h"

using namespace llvm;
using namespace llvm::orc;

int main(){
    
    QInfo info;
    QDMI_Session session = NULL;
    int err = QInfo_create(&info);

    err = QDMI_session_init(info, &session);

    std::vector<QDMI_Device> devices = FOMAC_available_devices(true);
    
    SMDiagnostic error;
    ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
    std::unique_ptr<Module> module = parseIRFile("/home/ubuntu/scheduler/inputs/bell_state.ll", error, *(TSCtx.getContext()));
    ThreadSafeModule TSM = ThreadSafeModule(std::move(module),std::move(TSCtx));

    std::vector<QuantumTask*> tasks;
    QuantumTask mQuantumTask;
    mQuantumTask.mThreadSafeModule =  std::move(TSM);
    mQuantumTask.mPreferredQpus.push_back(devices.at(0));
    mQuantumTask.mPreferredQpus.push_back(devices.at(1));
    mQuantumTask.mDuration = 5;
    mQuantumTask.mPriority = 1.;
    mQuantumTask.mTaskId = 0;
    tasks.push_back(&mQuantumTask);

    QuantumTask mQuantumTask2;
    ThreadSafeContext TSCtx2(std::make_unique<LLVMContext>());
    std::unique_ptr<Module> module2 = parseIRFile("/home/ubuntu/scheduler/inputs/example.ll", error, *(TSCtx2.getContext()));
    ThreadSafeModule TSM2 = ThreadSafeModule(std::move(module2),std::move(TSCtx2));
    mQuantumTask2.mThreadSafeModule =  std::move(TSM2);
    mQuantumTask2.mPreferredQpus.push_back(devices.at(0));
    mQuantumTask2.mPreferredQpus.push_back(devices.at(1));
    mQuantumTask2.mDuration = 5;
    mQuantumTask2.mPriority = 2.;
    mQuantumTask2.mTaskId = 1;
    tasks.push_back(&mQuantumTask2);


    Submiter2Device device2Submitter;
    for (const QDMI_Device& device : devices) {
        auto submitter = std::make_shared<Submitter>(device); // Create Submitter using smart pointer
        device2Submitter.emplace(device, submitter); // Store Submitter in map using smart pointer
    }

    scheduler(device2Submitter, tasks);


    delete device2Submitter.at(0).get();
    delete device2Submitter.at(0).get();
    
     
}
    