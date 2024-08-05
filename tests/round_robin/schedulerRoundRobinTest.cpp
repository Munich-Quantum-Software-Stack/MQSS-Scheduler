#include "iostream"
#include "round_robin.hpp"
#include "scheduler.hpp"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include <QuantumTask.hpp>
#include <Submitter.hpp>
#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <qdmi.h>
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

    // TODO: remove loop once we have multiple available devices
    for (int i = 0; i < 3; i++) {
      // Init and store Submitter in map for later use
      auto submitter = std::make_shared<Submitter>(device, 3);
      device2Submitter.emplace(device, submitter);
      // Init and store SchedulerQueue in vector for later use
      auto schedulerQueue = std::make_shared<SchedulerQueue>(submitter);
      queues.push_back(schedulerQueue);
    }
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

  std::vector<std::shared_ptr<QuantumTask>> tasks;

  for (int id = 0; id < 10; id++) {
    auto task = std::make_shared<QuantumTask>(id);
    tasks.push_back(task);
  }
  // Here the magic happens, the tasks are scheduled
  scheduler(queues, tasks);
  return 0;
}