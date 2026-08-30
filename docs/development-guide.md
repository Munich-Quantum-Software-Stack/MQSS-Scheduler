# Development Guide

This guide is for contributors extending the Scheduler, debugging policy behavior, and
maintaining documentation quality.

## Repository Layout

Key directories:

- `include/scheduler/`: public API — `scheduler.hpp` (`Scheduler<TaskType>`,
  `Schedulable`/`SizedSchedulable` concepts, `SchedulingPolicy`), `scheduler.tpp`
  (template method bodies), `quantum_task.hpp` (`mqss::QuantumTask`, an example
  `Schedulable` task type)
- `src/`: explicit template instantiation for `mqss::QuantumTask`
- `tests/`: example-based and property-based GoogleTest cases, one per policy
- `examples/qrm-sscheduler/`: gRPC ingress → Scheduler → QDMI submission pipeline
- `external/`: QDMI/QInfo/submitter sources and build scripts for the example toolchain
  (not needed to build or test the library itself)
- `docs/`: this Doxygen documentation site
- `cmake/`: `Find*.cmake` modules and `ExternalDependencies.cmake` (GoogleTest)

## Adding a Scheduling Policy

`Scheduler<TaskType>` dispatches by `SchedulingPolicy` — see `scheduler.hpp` for the
existing policies (first-in-first-out, priority-based, round-robin, backfilling,
mix-n-multi) as the reference shape for a new one. Any new policy should:

1. Only require `Schedulable` (or `SizedSchedulable`, if it needs qubit counts) — the
   scheduler stays task-type-agnostic by design.
2. Come with both an example-based test and a property-based test in
   `tests/gtest_scheduler.cpp`, matching the pattern already used for the other policies.

## Building and Testing

See [Getting Started](getting-started.md) for build, test, and documentation-generation
commands.

## Documentation

Public API documentation is generated from the headers in `include/scheduler/` via
Doxygen (see `docs/CMakeLists.txt` and `docs/Doxyfile.in`). Document new public types and
functions with Doxygen comments (`@brief`, `@param`, `@return`) so they show up in the
[API Reference](api-reference.md) — `WARN_IF_UNDOCUMENTED` is enabled, so an
undocumented public symbol shows up as a build warning when generating docs locally.
