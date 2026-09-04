<!----------------------------------------------------------------------------
Copyright (c) 2024 - 2026 Munich Quantum Software Stack
All rights reserved.

Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://llvm.org/LICENSE.txt

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
--------------------------------------------------------------------------->

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/_static/mqss_logo_dark.svg" width="20%">
    <img src="docs/_static/mqss_logo.svg" width="20%">
  </picture>
</p>

# MQSS Quantum Task Scheduler

<p align="center">
  <a href="https://munich-quantum-software-stack.github.io/MQSS-Scheduler/">
    <img style="min-width: 200px !important; width: 30%;" src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA0NDggNTEyIj48IS0tIUZvbnQgQXdlc29tZSBGcmVlIDYuNi4wIGJ5IEBmb250YXdlc29tZSAtIGh0dHBzOi8vZm9udGF3ZXNvbWUuY29tIExpY2Vuc2UgLSBodHRwczovL2ZvbnRhd2Vzb21lLmNvbS9saWNlbnNlL2ZyZWUgQ29weXJpZ2h0IDIwMjQgRm9udGljb25zLCBJbmMuLS0+PHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTk2IDBDNDMgMCAwIDQzIDAgOTZMMCA0MTZjMCA1MyA0MyA5NiA5NiA5NmwyODggMCAzMiAwYzE3LjcgMCAzMi0xNC4zIDMyLTMycy0xNC4zLTMyLTMyLTMybDAtNjRjMTcuNyAwIDMyLTE0LjMgMzItMzJsMC0zMjBjMC0xNy43LTE0LjMtMzItMzItMzJMMzg0IDAgOTYgMHptMCAzODRsMjU2IDAgMCA2NEw5NiA0NDhjLTE3LjcgMC0zMi0xNC4zLTMyLTMyczE0LjMtMzIgMzItMzJ6bTMyLTI0MGMwLTguOCA3LjItMTYgMTYtMTZsMTkyIDBjOC44IDAgMTYgNy4yIDE2IDE2cy03LjIgMTYtMTYgMTZsLTE5MiAwYy04LjggMC0xNi03LjItMTYtMTZ6bTE2IDQ4bDE5MiAwYzguOCAwIDE2IDcuMiAxNiAxNnMtNy4yIDE2LTE2IDE2bC0xOTIgMGMtOC44IDAtMTYtNy4yLTE2LTE2czcuMi0xNiAxNi0xNnoiLz48L3N2Zz4=" alt="Documentation" />
  </a>
  &nbsp;
  <a href="https://github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler/actions/workflows/ci.yml">
    <img src="https://github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler/actions/workflows/ci.yml/badge.svg" alt="CI" />
  </a>
</p>

A dependency-free, header-driven C++20 task scheduler (`mqss::scheduler::Scheduler<TaskType>`) for
dispatching quantum circuits to quantum hardware devices. `TaskType` is constrained structurally by the Schedulable concept — any type exposing `task_id()`/`priority()` satisfies it; plug in your own task struct and the scheduler works unmodified.

## FAQ

### What is MQSS?

_MQSS_ stands for _Munich Quantum Software Stack_ and is a project of the _Munich Quantum Valley_
initiative. It is jointly developed by the _Munich Quantum Valley (MQV) gGmbH_, _Leibniz
Supercomputing Centre (LRZ)_, the _Chair for Design Automation (CDA)_, and the _Chair of Computer
Architecture and Parallel Systems (CAPS)_ at TUM. It provides a comprehensive compilation and
runtime infrastructure for on-premise and remote quantum devices, support for modern compilation and
optimization techniques, and enables both current and future high-level abstractions for quantum
programming. This stack is designed to be capable of deployment in a variety of scenarios via
flexible configuration options. This includes stand-alone scenarios for individual systems, cloud
access to a variety of devices, as well as tight integration into HPC environments supporting
quantum acceleration. Concrete instances of the _MQSS_ are deployed at the LRZ and MQV gGmbH,
providing unified access to all of their quantum devices through multiple compatible access paths.
This includes a web portal, command line access via web credentials, as well as the option for
hybrid access with tight integration with HPC systems. It facilitates the connection between
end-users and quantum computing platforms by its integration within HPC infrastructures, such as
those found at the LRZ.

### Where is the code?

The code is publicly available and hosted on GitHub at
[github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler](https://github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler).

### Under which license is the MQSS Quantum Task Scheduler released?

The **MQSS Quantum Task Scheduler** is released under the Apache License v2.0 with LLVM Exceptions.
See [LICENSE](LICENSE) for more information. Any contribution to the project is assumed to be under
the same license.

## Architecture

![Scheduler Architecture](docs/figures/mqss-scheduler-models.webp)

`Scheduler<TaskType>` is constrained by the `Schedulable` concept (`task_id()`, `priority()`). It queues tasks
(`scheduleTask`/`scheduleTasks`) and dispatches them (`getNextReadyTask`) according to
a `SchedulingPolicy` fixed at construction — see [Scheduling Policies](#scheduling-policies).

`include/scheduler/quantum_task.hpp`'s `mqss::scheduler::QuantumTask` is a lightweight example task
type (mirroring the MQSS protocol's `QuantumTask` message) used to exercise the
scheduler in tests and examples. QDMI/LLVM/submitter enter the picture of quantum resource manager at the _example_
(`examples/qrm-sscheduler/`), where a scheduled task actually gets submitted to a device.

## Requirements

- CMake ≥ 3.19, a C++20-capable compiler
- GoogleTest, fetched automatically when building the test suite (see
  [cmake/ExternalDependencies.cmake](cmake/ExternalDependencies.cmake))
- Doxygen + Graphviz (optional, only needed to generate the API documentation)

`external/`'s dependencies (QDMI, QInfo, submitter, LLVM) are required to build the
example/submission toolchain (`examples/qrm-sscheduler/`) alongside the library.

## Building

### 1. Configure and build the scheduler

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SCHEDULER_TESTS=ON
cmake --build build -j
```

### 2. (Optional) Build the example/submission toolchain

If you also need the gRPC ingress → Scheduler → QDMI submission pipeline
(`examples/qrm-sscheduler/`), clone and build the external dependencies first:

```bash
cd external
bash clone.sh    # QDMI, QInfo, submitter
bash build.sh    # compile and install the libraries
```

See [Getting Started](docs/getting-started.md) for the full step-by-step guide.

## Running Tests

```bash
ctest --test-dir build --output-on-failure
```

## Examples

```bash
source examples/export_env_vars.sh
cd examples/qrm-sscheduler
make proto && make
make run
```

See [`examples/qrm-sscheduler/Readme.md`](examples/qrm-sscheduler/Readme.md) for the full
gRPC → `Scheduler` → QDMI pipeline, including how to submit a quantum circuit and verify the
result.

## Contact

Please use the GitHub channels —
[issues](https://github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler/issues) and
[pull requests](https://github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler/pulls) —
to allow for open discussion.

## Contributing

See [contributing.md](contributing.md) and [code_of_conduct.md](code_of_conduct.md).

## License

This project is released under the Apache License v2.0 with LLVM Exceptions. See
[LICENSE](LICENSE) for more information. Any contribution to the project is assumed to be under
the same license.
