/*
 * Copyright 2024 - 2026 Munich Quantum Software Stack
 * All rights reserved.
 *
 * Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
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

    char port_name[MPI_MAX_PORT_NAME];
    MPI_Comm intercomm;

    // Open port and print it
    MPI_Open_port(MPI_INFO_NULL, port_name);
    printf("MQSS-Scheduler: opened port = %s\n", port_name);
    fflush(stdout);  // Make sure it's flushed for the client to read

    // Accept connection
    MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm);

    // Receive message from client
    char message[100];
    MPI_Recv(message, 100, MPI_CHAR, 0, 0, intercomm, MPI_STATUS_IGNORE);
    printf("MQSS-Scheduler received: %s\n", message);

    MPI_Comm_disconnect(&intercomm);
    MPI_Close_port(port_name);
    MPI_Finalize();

    return 0;
}
