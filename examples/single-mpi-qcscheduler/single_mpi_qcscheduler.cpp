#include <mpi.h>
#include <unistd.h>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>

#include <qinfo/qinfo.h>
#include <qdmi.hpp>
#include <qdmi/constants.h>

#include <submitter.hpp>
#include <quantum_task.hpp>

#include <scheduler/scheduler.hpp>
#include <scheduler/queue/queue.hpp>

// --------------------------------------------------------------
// Global variables
// --------------------------------------------------------------
int NUM_QJOBS = 5;

// --------------------------------------------------------------
// Serialize job as string (for sending over MPI)
// --------------------------------------------------------------
std::string serialize_quantum_task(std::shared_ptr<QuantumTask> qtask) {
    std::ostringstream oss;
    oss << qtask->TaskId << ";" << qtask->NumberQbits << ";" << qtask->NumberShots
        << ";" << qtask->CircuitFile << ";" << qtask->CircuitFileType << ";" << qtask->ResultsDestination;

    return oss.str();
}

// --------------------------------------------------------------
// Deserialize job from string
// --------------------------------------------------------------
std::shared_ptr<QuantumTask> deserialize_quantum_task(std::string &s) {
    std::istringstream iss(s);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(iss, token, ';')) parts.push_back(token);

    auto qtask = std::make_shared<QuantumTask>(std::stoi(parts[0]));
    qtask->NumberQbits = std::stoi(parts[1]);
    qtask->NumberShots = std::stoi(parts[2]);
    qtask->CircuitFile = parts[3];
    qtask->CircuitFileType = parts[4];
    qtask->ResultsDestination = parts[5];

    return qtask;
}


int main(int argc, char **argv) {

    // --------------------------------------------------------------
    // MPI Communicator Setup
    // --------------------------------------------------------------
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 3) {
        if (rank == 0) std::cerr << "Need at least 3 MPI processes.\n";
        MPI_Finalize();
        return 1;
    }

    // --------------------------------------------------------------
    // Process 0: Job Generator and Submission as receiving quantum tasks from MQP/HPCQC
    // --------------------------------------------------------------
    if (rank == 0) {

        int num_qjobs = NUM_QJOBS;
        for (int i = 0; i < num_qjobs; i++) {
            auto qtask = std::make_shared<QuantumTask>(i);
            qtask->NumberQbits = 2;
            qtask->NumberShots = 10000;
            qtask->CircuitFile = "qcircuit_"+std::to_string(i)+".qasm";
            qtask->CircuitFileType = "qasm3";
            qtask->ResultsDestination = "output_qcircuit_"+std::to_string(i)+".txt";

            std::string msg = serialize_quantum_task(qtask);
            MPI_Send(msg.c_str(), msg.size() + 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        }
        printf("[P%d] Num. total jobs: %d submitted\n", rank, num_qjobs);
    
    // --------------------------------------------------------------
    // Process 1: Scheduler Component
    // --------------------------------------------------------------
    } else if (rank == 1) {

        // Initialize the MQSS Scheduler
        printf("[P%d] Initializing MQSS Scheduler\n", rank);
        Scheduler mqss_scheduler;
        mqss_scheduler.initScheduler();

        // Receive quantum tasks from MQP/HPCQC
        char buffer[1024];
        int num_revc_jobs = NUM_QJOBS;
        MPI_Status sched_status;
        for (int i = 0; i < num_revc_jobs; i++) {
            MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &sched_status);
            std::string msg(buffer);
            auto qtask = deserialize_quantum_task(msg);
            printf("[P%d] Scheduler: Received job %d - %s\n", rank, qtask->TaskId, qtask->CircuitFile.c_str());
            mqss_scheduler.sched_queue->addJob(qtask);
        }
        printf("[P%d] Num. queued jobs: %d submitted\n", rank, mqss_scheduler.sched_queue->num_total_jobs);

        // Simulate scheduling decision and forward to submitter (rank 2)
        std::string sched_msg_to_submitter = serialize_quantum_task(mqss_scheduler.sched_queue->jobs.front());
        MPI_Send(sched_msg_to_submitter.c_str(), sched_msg_to_submitter.size() + 1, MPI_CHAR, 2, 0, MPI_COMM_WORLD);

        // Finalize the scheduler thread
        mqss_scheduler.finiScheduler();

    // --------------------------------------------------------------
    // Process 2: Submitter Component & QDMI Device
    // --------------------------------------------------------------
    } else {

        // Receive quantum tasks from Scheduler
        char submitter_buffer[1024];
        int num_revc_sched_jobs = 1;
        MPI_Status submiter_status;
        MPI_Recv(submitter_buffer, sizeof(submitter_buffer), MPI_CHAR, 1, 0, MPI_COMM_WORLD, &submiter_status);
        std::string msg_from_scheduler(submitter_buffer);
        auto qtask = deserialize_quantum_task(msg_from_scheduler);
        printf("[P%d] Submitter: Received job %d - %s\n", rank, qtask->TaskId, qtask->CircuitFile.c_str());

        // ----------------------------------------------------------
        // Setting and get the QDMI device info
        // Note: ensure the related dynamic libraries are loaded
        // and QDMI config file is set
        // ----------------------------------------------------------
        std::string driver_name = "qdmi_example_driver";
        std::string token = "test_token";
        auto params = std::make_tuple(driver_name, token);
        std::unique_ptr<qdmi::Session> session;
        session = std::make_unique<qdmi::Session>(std::get<0>(params), std::get<1>(params));
        std::vector<qdmi::Device> devices = session->get_devices();
        for (const auto &device : devices) {
            printf("[P%d] QDMI: CxxQDMISession_GetDeviceName\n", rank);
        }

    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}