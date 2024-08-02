#ifndef SCHEDULER_QUEUE_H
#define SCHEDULER_QUEUE_H

#include <QuantumTask.hpp>
#include <Submitter.hpp>
#include <deque>
#include <memory>

class SchedulerQueue : public ISubmitterObserver,
                       public std::enable_shared_from_this<SchedulerQueue> {
public:
  // Pointer to a Submitter object
  std::shared_ptr<Submitter> mpSubmitter;

  // Total duration of all tasks in the queue
  double mTotalDuration = 0;

  // Deque to hold the tasks in the queue
  std::deque<std::shared_ptr<QuantumTask>> mTasks = {};

  // Constructor that takes a pointer to a Submitter object
  explicit SchedulerQueue(std::shared_ptr<Submitter> submitter)
      : mpSubmitter(submitter) {}

  // Must not be done in the constructor
  void initialize() { mpSubmitter->addObserver(shared_from_this()); }

  // Method to add a task at a specific position in the queue
  int addTask(std::shared_ptr<QuantumTask> quantumTask, int position);

  // Method to remove a specific task from the queue
  int removeTask(std::shared_ptr<QuantumTask> quantumTask);

  // Method to update the SchedulerQueue state when a task is popped from the
  // Submitter's queue
  void onTaskPopped(std::shared_ptr<QuantumTask> quantumTask) override {
    removeTask(quantumTask);
  }
};

#endif