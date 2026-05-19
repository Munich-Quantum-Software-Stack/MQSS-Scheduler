#ifndef SUBMITTER_HPP
#define SUBMITTER_HPP

#include "quantum_task.hpp"

// #include <qdmi/client.h>
#include <qdmi/constants.h>
#include <qdmi.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * @brief The ISubmitterObserver class, observing task submission
 * 
 * An observer interface for the Submitter class. It is used to notify
 * observers when a task is popped from the queue. The observer should
 * implement the onTaskPopped method to handle the task.
 */
class ISubmitterObserver {
  public:
    // To be implemented by an observer(SchedulerQueue)
    virtual void onTaskPopped(std::shared_ptr<QuantumTask> task) = 0;
};

/**
 * @brief The CompareExecutionOrder struct, implementing comparison operators on
 * task submission orders.
 * 
 * An operator to compare two QuantumTask objects based on their execution
 * order. It is used to sort the task queue in the Submitter class.
 */
struct CompareExecutionOrder {
  bool operator()(const std::shared_ptr<QuantumTask> lhs,
                  const std::shared_ptr<QuantumTask> rhs) const {
    return lhs->ExecutionOrder > rhs->ExecutionOrder;
  }
};

/**
 * @brief The Submitter class, prividing attributes and methods regarding
 * quantum task submission to a QDMI device.
 * 
 * A Submitter manages the submission of quantum tasks to a QDMI device. Each QDMI device
 * can have its own Submitter pining to a thread, which handles the task queue and submission process.
 * The Submitter can enqueue tasks, dequeue them, and submit them to the QDMI device.
 */
class Submitter {

  private:
    string device_name;                 // The name of the QDMI device
    int sub_queue_size;                 // The size of the submitter queue 
    std::queue<std::shared_ptr<QuantumTask>> sub_queue; // The quantum task queue at submitter
    std::mutex qlock;                   // Mutex lock for the queue
    std::condition_variable condition;  // The condition for the queue lock
    std::thread submitter_thread;       // The thread for the submitter
    bool stop;                          // The stop flag
    std::vector<std::shared_ptr<ISubmitterObserver>> observers; // List of observers

  public:
    Submitter();
    Submitter(qdmi::Device device);
    ~Submitter();

    void initSubmitter(qdmi::Device device);
    void finiSubmitter(qdmi::Device device);
    void enqueue(std::shared_ptr<QuantumTask> task);
    int getQueueSize();
    QDMI_Job_Status submitTask(std::shared_ptr<QuantumTask> task, qdmi::Device device);

    void addObserver(std::shared_ptr<ISubmitterObserver> observer);
    void removeObserver(std::shared_ptr<ISubmitterObserver> observer);
};

#endif