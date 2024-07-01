
#include <round_robin.hpp>
#include "QuantumTask.hpp"
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


QuantumTask createTask(std::string benchmarkAdress, QDMI_Device device){

    SMDiagnostic error;
    ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
    std::unique_ptr<Module> module = parseIRFile(benchmarkAdress, error, *(TSCtx.getContext()));
    ThreadSafeModule TSM = ThreadSafeModule(std::move(module),std::move(TSCtx));

    QuantumTask mQuantumTask;
    mQuantumTask.mThreadSafeModule =  std::move(TSM);
    mQuantumTask.mPreferredQpus.push_back(device);
    mQuantumTask.mDuration = 5;
    mQuantumTask.mPriority = 1.;
    mQuantumTask.mTaskId = 0;

    return mQuantumTask;
}

int main(){
    
    QInfo info;
    QDMI_Session session = NULL;
    int err = QInfo_create(&info);

    err = QDMI_session_init(info, &session);

    std::vector<QDMI_Device> devices;
    int count;
    QDMI_core_device_count(&session, &count);

    for(int i = 0; i < count; i++){
        QDMI_Device device;
        QDMI_core_open_device(&session, i , &info, &device);
        devices.push_back(device);
    }


    std::vector<QuantumTask> tasks;
    
    tasks.push_back(createTask("/home/ubuntu/scheduler/inputs/bell_state.ll", devices.at(0)));
    tasks.push_back(createTask("/home/ubuntu/scheduler/inputs/example.ll", devices.at(0)));


    Device2SubmitterType device2Submitter;
    for (const QDMI_Device& device : devices) {
        auto submitter = std::make_shared<Submitter>(device); 
        device2Submitter.emplace(device, submitter); 
    }

    scheduler(device2Submitter, &tasks);

    for(QuantumTask& task : tasks){
        QDMI_Device dev = task.mScheduledQpu;
        device2Submitter[dev]->acceptATask(&task);
    }
    
}
    