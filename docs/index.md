# MQSS Quantum Task Scheduler

A dependency-free, header-driven C++20 task scheduler (`mqss::scheduler::Scheduler<TaskType>`) for
dispatching quantum circuits to quantum hardware devices via
[QDMI](https://github.com/Munich-Quantum-Software-Stack/QDMI). Any task type exposing
`task_id()`/`priority()` works — the library itself has no coupling to a specific task
struct, compiler pipeline, or logging framework.

# Concepts

## Schedulable Task Types

- `Schedulable`: minimum requirement, a `task_id()`/`priority()`-exposing type
- `SizedSchedulable`: `Schedulable` plus a qubit-count accessor, needed by policies that
  pack multiple tasks per device (backfilling, mix-n-multi)

## Scheduling Policies

- First-In-First-Out
- Priority-based
- Round-robin
- Backfilling
- Mix-n-multi

### Getting started

Use [Getting Started](getting-started.md) to build the library and run its tests.

### Development

Use [Development Guide](development-guide.md) for repository layout and contribution
workflow.

### API Reference

Detailed [API documentation](api-reference.md).
