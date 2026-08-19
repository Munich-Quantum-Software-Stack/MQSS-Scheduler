# qrm-sscheduler: Slurm → gRPC → Scheduler → QDMI → Aer

A gRPC ingress pipeline built on this repo's [`Scheduler`](../../include/scheduler/scheduler.hpp)
(`mqss::Scheduler<mqss::QuantumTask>`) and its lightweight `mqss::QuantumTask` type, mirroring
`mqss.protocol.v1.QuantumTask`. This is where to look for how `Scheduler` fits into a real
submission pipeline, rather than just its own unit tests
([`../../tests/gtest_scheduler.cpp`](../../tests/gtest_scheduler.cpp)).

```
hpc-container (Slurm + mqss-spank-plugin)
    -> sbatch job runs bell_state_offload.py --provider mqss-grpc
    -> gRPC over hpcqc-net
qc-container
    -> qrm_sscheduler rank 0: gRPC listener, writes circuit to /tmp/mqss-grpc-circuits/,
       builds a mqss::QuantumTask, forwards it to rank 1
    -> rank 1: Scheduler<mqss::QuantumTask> (FirstInFirstOut) queues incoming tasks;
       an embedded std::thread inside the same rank owns the QDMI session + Submitter,
       polling getNextReadyTask() and submitting whatever comes back
    -> QDMI fake device (CXX_QDMI_device_job_submit) shells out to run_circuit.py
    -> run_circuit.py: real Aer simulation, writes counts back
    -> Submitter writes real counts to ResultsDestination on disk
```

## Architecture

This follows QRM's own
[`deployment/workflow-aio/main.cpp`](
https://github.com/Munich-Quantum-Software-Stack/QRM/blob/qrm-ci/deployment/workflow-aio/main.cpp#L23)
pattern: a scheduler with a background thread that owns the QDMI submission path, decoupled
from however tasks arrive. QRM runs that as two `std::thread`s in one process; here it's 2
MPI ranks (rank 0 = gRPC ingress, rank 1 = `Scheduler` + an embedded submitter thread).

One QDMI device, submitted to from one thread — no per-alias fan-out or `qc_alias`-based
routing. `mqss::QuantumTask` carries `preferred_qpu`/`scheduled_qpu` (mirroring the real
protobuf schema), but nothing in this example reads them yet.

## The adapter point

`Submitter::submitTask` (this repo's own, `external/submitter/`) still expects the
older, LLVM/JIT-coupled global `QuantumTask` struct, not `Scheduler`'s `mqss::QuantumTask` —
see [`include/scheduler/quantum_task.hpp`](../../include/scheduler/quantum_task.hpp)'s header
comment for why these are two separate types. `qrm_sscheduler.cpp`'s `to_submitter_task()`
converts one into the other at the one point they need to meet — right where the embedded
submitter thread pulls a ready task off `Scheduler` and is about to call `submitTask`.

## Build & run standalone (inside `qc-container`)

```bash
docker compose -f dev-container/docker-compose.yml exec qc-container bash
cd /home/admin/shared/MQSS-Scheduler
source examples/export_env_vars.sh
cd examples/qrm-sscheduler
make proto   # regenerate quantum_job.pb.{h,cc}/quantum_job.grpc.pb.{h,cc} - not committed
make
make run     # mpirun (shared-memory-only MCA flags) -np 2 ./qrm_sscheduler
```

From another shell in the same container, submit a real job:

```bash
cd /home/admin/shared/MQSS-Scheduler/slurm-spank-plugins/mqss-spank-plugin/test
uv run --script bell_state_offload.py --provider mqss-grpc --target localhost:50051 --shots 200
```

Then check the real result on disk:

```bash
cat /tmp/mqss-grpc-circuits/job_0_result.txt
# e.g. 00:96,11:104 - a genuine Bell-state distribution, not random noise across
# all bit patterns.
```

## Known gaps

- No results round trip over gRPC — `SubmitCircuit` returns as soon as the task is queued
  with `Scheduler`, not after execution completes.
- Per-task dispatch, not real windowed batching/backfill.
- `quantum_job.proto`'s `CircuitRequest` has no qubit-count or priority field, so every task
  arrives with `n_qbits=1` (the `mqss::QuantumTask` default) and `priority=0`. `Scheduler` is
  constructed with `SchedulingPolicy::FirstInFirstOut` accordingly — with every task at the
  same priority there's nothing for `PriorityBased` (or `RoundRobin`/`Backfilling`/
  `MixNMulti`) to differentiate on yet, unlike QRM's own workflow-aio, which defaults to
  `PriorityBased`.
- Auth (`MQSS_TOKEN`/`MQSS_TOKENS_FILE`, injected by `mqss-spank-plugin`) is not forwarded
  into `CircuitRequest` or checked here.
- Single QDMI device only, picked as `devices[0]` (device order from `get_devices()` is not
  deterministic) — no alias-based device selection or multi-device concurrency.
- Graceful shutdown (`sigwait()`-based, see `qrm_sscheduler.cpp`'s top-of-file comment) has
  been observed to not reliably trigger when `SIGINT`/`SIGTERM` is sent to the `mpirun`
  process externally (e.g. `kill -TERM <mpirun-pid>`) — ranks end up killed by mpirun's own
  `--mca odls_base_sigkill_timeout` instead, and `mpirun` itself can hang afterward. Not yet
  root-caused; whether a real interactive Ctrl+C behaves differently is untested.
- Always run via `make run`, not a bare `mpirun -np 2 ./qrm_sscheduler` — see the Makefile's
  comments on the shared-memory-only MCA transport flags.
