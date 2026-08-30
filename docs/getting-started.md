# Getting Started

This guide covers building, testing, and generating documentation for the MQSS Quantum
Task Scheduler.

## Prerequisites

- CMake 3.19 or newer
- A C++20-capable compiler
- Doxygen (optional, only required for API docs generation)

Building `Scheduler` itself needs nothing beyond a C++20 compiler and GoogleTest (fetched
automatically, see
[cmake/ExternalDependencies.cmake](https://github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler/blob/develop/cmake/ExternalDependencies.cmake)).
`external/`'s dependencies (QDMI/QInfo/submitter/LLVM) are only needed for the
example/submission toolchain wired up alongside it — not for the library or its tests.

## Configure and Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install
```

## Running Tests

```bash
cd build
ctest --output-on-failure
```

`gtest_scheduler` needs nothing but GoogleTest — no QDMI/LLVM or extra environment setup
required.

## Building This Documentation

```bash
cmake .. -DBUILD_SCHEDULER_DOCS=ON
cmake --build . --target docs
```

Generated HTML lands under `build/docs/html/index.html`.

## Building the Example/Submission Toolchain

If you also need the gRPC ingress → Scheduler → QDMI submission pipeline
(`examples/qrm-sscheduler/`), clone and build the external dependencies first:

```bash
cd external
bash clone.sh    # QDMI, QInfo, submitter
bash build.sh    # installs everything under external/installed/
cd ..
source examples/export_env_vars.sh
```

See the
[top-level README](https://github.com/Munich-Quantum-Software-Stack/MQSS-Scheduler/blob/develop/README.md)
for the full architecture and dependency details.
