# MQSS Quantum Task Scheduler
[comment]: <Branch: minh/ssintegrat - Integration of quantum task scheduler and device selector>

## Overview
The **MQSS Quantum Task Scheduler** is a unified module for scheduling quantum tasks prior to dispatch on quantum hardware. It provides a flexible, modular design supporting multiple queue types, device selection strategies, and scheduling algorithms.

---

## Requirement
The MQSS scheduler has some dependencies. Please make sure Cmake can find these packages/libraries installed, i.e., LLVM, submitter, QDMI.

[comment]: <First, sudo apt update>

[comment]: <Currently, llvm is installed from the Ubuntu distribution, apt-get install clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev libc++abi1 libclang-dev libclang1 liblldb-dev libllvm-ocaml-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm>

[comment]: <The llvm version is llvm-18>

---

## 🧱 Core Components

- **Queue**  
  Provides the `SchedulerQueue` class, which observes `SubmitterQueue`s (defined in the [Submitter](https://github.com/Munich-Quantum-Software-Stack/submitter) library).

- **Selector**  
  Chooses a suitable quantum device based on circuit characteristics or processing passes.

- **Scheduler**  
  Implements various algorithms for managing task execution on queues.

- **Linked Components**
  + Submitter
  + QDMI

---

## Available Device Selectors

* Supervised Predictor:
  - Random Forest-based Predictor
  - MLP-based Predictor
  - SVM-based Predictor
  - TBA ...
* Unsupervised Predictor:
  - Genome-based Predictor
  - RL-based Predictor
  - TBA ...

## Available Scheduling Algorithms

- Backfilling
- Round Robin
- and to be more ...

---

## 📦 Requirements

- Currently use [ONNX Runtime](https://onnxruntime.ai/) as a proxy for training and loading ML/DL models in the selector, to be updated.

---

## Project Structure
```bash
scheduler/
├── CMakeLists.txt
├── cmake/
├── selector/
├── include/
│   ├── scheduler/
│       ├── scheduler.hpp
│       ├── queue/
│       ├── utils/
├── src/
│   ├── scheduler.cpp
│   ├── queue/
│   ├── utils/
├── examples/
├── test/
│   ├── test_scheduler.cpp
└── .clang-format
└── .gitignore
└── README.md
```

---

## 🔧 Installation
