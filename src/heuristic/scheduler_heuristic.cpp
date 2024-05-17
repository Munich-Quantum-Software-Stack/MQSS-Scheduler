/**
 * @file scheduler.cpp
 * @brief Implementation of a ML guided scheduler.
 */
#include <heuristic.hpp>
#include "Submitter.hpp"
#include "predictor.hpp"
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>




//using llvm::orc::ThreadSafeModule;
/**
 * @brief Calculate scores for the devices. Either based on user preference, ML
 * model, or both.
 * @param task The QuantumTask to be scheduled.
 * @return Scores for the devices.
 */
std::unordered_map<My_QDMI_Device, float> calculate_scores(QuantumTask *task)
{
    std::unordered_map<My_QDMI_Device, float> scores;

    // User only wants to use a single QPU
    if (task->mPreferredQpus.size() == 1)
    {
        // maximum score for the only QPU
        scores = {{task->mPreferredQpus.front(), 1.0}};
    }
    // TODO
    // else if (user wants to use some QPUs more than others):
    //      scores = task.preferred_qpus
    else
    { // If choice is not forced or the user preference is equally distributed
        
        // TODO: once My_QDMI_Device can be ID'd by a string, use it to select the model
        std::vector<std::string> models;
        for (const auto& qpu : task->mPreferredQpus) {
            models.push_back("q20_ga_critical_depth");
        }

        // Predict figure of merit for every device
        std::map<std::string, float> predictions = predict(task->mThreadSafeModule, models);

        std::unordered_map<My_QDMI_Device, float> scores;
        for (const auto& device : task->mPreferredQpus) {
            scores[device] = predictions["q20_ga_critical_depth"];
        }
    }

    std::cout << "   [Scheduler]...........Scores: ";
    for (auto &score : scores)
    {
        std::cout << " " << score.second << " " << std::endl;
    }
    return scores;
}

/**
 * @brief Select the shortest queue among the top 3 scored devices.
 * @param task The QuantumTask to be scheduled.
 * @param scores The scores of the devices.
 * @return The name of the selected device.
 **/
 
 My_QDMI_Device choose_device(QuantumTask* task,
                          const std::unordered_map<My_QDMI_Device, float> &scores, 
        Submiter2Device device2Submitter)
{
    // Find the devices with the three highest final scores
    std::vector<My_QDMI_Device> devices;
    for (auto &score : scores)
    {
        if (devices.size() < 3)
        {
            devices.push_back(score.first);
        }
        else
        {
            for (auto &device : devices)
            {
                if (score.second > scores.at(device))
                {
                    device = score.first;
                    break;
                }
            }
        }
    }

    std::cout << std::endl << "   [Scheduler]...........Choosing target My_QDMI_Device from"
              << " the following devices: ";
    for (auto &device : devices)
    {
        // std::cout << device->library.libname << std::endl;
        std::cout << device.mName << std::endl;
    }

    // Get current queue from metadata
    //QirPassRunner &QPR = QirPassRunner::getInstance();
    //QirMetadata &qirMetadata = QPR.getMetadata();

    // Find the queue with shortest end time among the three devices
    float min_end_time = std::numeric_limits<float>::max();
    My_QDMI_Device target_device;

    for (auto &device : devices)
    {
        std::shared_ptr<Submitter> submitter = device2Submitter[device];

        std::deque<QuantumTask *> mTasks = submitter->mTasks;
        //QDMI_Queue queue = My_QDMI_Device_get_queue(device);
        //qirMetadata.get_queue(device);
        int queueSize = submitter->getQueueSize();
        if (queueSize == 0)
        {
            return device;
        }
        QuantumTask* last_task = ( QuantumTask* )mTasks[queueSize -1];
        float end_time = last_task->mEnd;
        //qirMetadata.get_end(queue->tasks.back()->task_id);
        if (end_time < min_end_time)
        {
            min_end_time = end_time;
            return device;
        }
    }
}
 
 
/**
 * @brief Schedule a QuantumTask on a target device using skipping strategy.
 * @param new_task The QuantumTask to be scheduled.
 * @param target_device The target device to schedule the QuantumTask on.
 * @return True if the QuantumTask was successfully scheduled, false otherwise.
 */
bool skipping_schedule(QuantumTask *new_task, My_QDMI_Device target_device, Submiter2Device device2Submitter)
 {
     // Get current queue from metadata
     //QirPassRunner &QPR = QirPassRunner::getInstance();
     //QirMetadata &qirMetadata = QPR.getMetadata();

     std::shared_ptr<Submitter> submitter = device2Submitter[target_device];
     
     //qirMetadata.get_queue(target_device);
     float new_task_duration = new_task->mDuration;
     int new_task_priority = new_task->mPriority;
     //qirMetadata.get_priority(new_task.task_id);

     // Age increment after already queued task was skipped by new_task
     // e.g. value of 1/2: integer priority level will increase after 2 skips
     float age_increment = 0.5;

     std::cout << "   [Scheduler]...........Inserting QuantumTask with ID "
               << new_task->mTaskId << " into the queue for device "
               // << target_device->library.libname << std::endl;
                << target_device.mName << std::endl;



     // Check if the queue is empty
     int insert_at = 0;
     int i = 0;
     int queueSize = submitter->getQueueSize();
     std::deque<QuantumTask *> tasks = submitter->mTasks;
     for (i = queueSize - 1; i >= 0; --i)
     {
         //QuantumTask &last_task = *queue->tasks[i];
         QuantumTask* last_task = (QuantumTask*) tasks[i];
         float last_task_end = last_task->mEnd;
         float last_task_priority = last_task->mPriority;
             //qirMetadata.get_priority(last_task.task_id);

         float predicted_end = last_task_end + new_task_duration;

         if (new_task_priority >
             std::floor(last_task_priority + last_task->mAge))
         {
             // always skip lower priority tasks
             if(predicted_end > last_task->mEnd){
                 last_task->mEnd = predicted_end;
             }

             last_task->mAge += age_increment;
             //qirMetadata.update_end(last_task.task_id, predicted_end);
             // increase age of skipped task
             //last_task.age = last_task.age + age_increment;
             continue; // check next job in line
         }
         else if (new_task_priority == last_task_priority)
         {
             //int last_parent_id = (last_task->parent_id == -1)
             //                            ? last_task->task_id
             //                            : last_task->parent_id;

             QuantumTask* parent_task = last_task->pParentTask  != NULL ? last_task->pParentTask : last_task;
             float last_parent_end = parent_task->mEnd;
             //qirMetadata.get_end(last_parent_id);

             if (predicted_end < last_parent_end)
             {
                 // can skip in line (wo delaying other task)

                 QuantumTask* new_parent_task = new_task->pParentTask  != NULL ? new_task->pParentTask : new_task;
                 //float new_parent_id = (new_task.parent_id == -1)
                 //                            ? new_task.task_id
                 //                            : new_task.parent_id;
                
                 //float new_parent_end = qirMetadata.get_end(new_parent_id);
                 float new_parent_end = new_parent_task->mEnd;
                 if (new_parent_end < last_parent_end)
                 {
                     // should skip in line (for overall speedup)
                     if(predicted_end > last_task->mEnd){
                         last_task->mEnd = predicted_end;
                     }
                     // increase age of skipped task
                     //last_task.age = last_task.age + age_increment;

                     last_task->mAge += age_increment;
                     continue; // check next job in line
                 }
             }
         }
         // no (more) skipping
         break;
     }
     // insert new_task at position i
     submitter->acceptATask(new_task, i);
     //QDMI_queue_insert_tasks(target_device, (void*)&new_task, i);
     //QDMI_queue_insert_tasks(queue, (void*)&new_task, i);
     //queue->insertTask(i, &((void*)new_task), new_task_duration);
     if(new_task_duration > new_task->mEnd){
         new_task->mEnd = new_task_duration;
     }
     return true;
 } 

/**
 * @brief Entry point for the scheduler.
 * @param task The QuantumTask to be scheduled.
 * @return The selected device on which the task was scheduled.
 */
extern "C" int scheduler(Submiter2Device device2Submitter, std::vector<QuantumTask *> tasks)
{
    // TODO uncomment when FOMAC is available
    //std::vector<My_QDMI_Device> devices = FOMAC_available_devices();
    //std::vector<std::string> devices = {"Q5", "Q20", "Q50"};

    std::cout << "   [Scheduler]..........." << device2Submitter.size()
              << " available device(s)" << std::endl;

    // Sort tasks by priority and within that by duration
    std::sort((tasks).begin(), (tasks).end(),
              [](const QuantumTask* a, const QuantumTask* b)
              {
                  if (a->mPriority == b->mPriority)
                  {
                      return a->mDuration > b->mDuration;
                  }
                  return a->mPriority > b->mPriority;
              });

    // Queue each task
    for (QuantumTask* task : tasks)
    {
        // Calculate scores to produce device ranking
        std::unordered_map<My_QDMI_Device, float> scores = calculate_scores(task);

        // Choose the device with the shortest queue out of top 3
        My_QDMI_Device target_device = choose_device(task, scores, device2Submitter);

        // Queue the task on the chosen device and skip if necessary
        bool success = skipping_schedule(task, target_device, device2Submitter);

        // TODO once FOMAC is available, set the QPU
        // task.scheduled_qpu = target_device;
    }

    std::cout << "   [Scheduler]...........returning selected device."
              << std::endl;

    return 0;
}
