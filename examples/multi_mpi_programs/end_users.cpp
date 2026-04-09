#include <mpi.h>

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

int main(int argc, char *argv[]) {
    
    MPI_Init(&argc, &argv);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port_name>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    char *port_name = argv[1];
    MPI_Comm intercomm;

    // Connect to server
    MPI_Comm_connect(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm);

    // Send message to server
    const char *msg = "Hello from End-Users!";
    MPI_Send(msg, strlen(msg) + 1, MPI_CHAR, 0, 0, intercomm);

    MPI_Comm_disconnect(&intercomm);
    MPI_Finalize();

    return 0;
}
