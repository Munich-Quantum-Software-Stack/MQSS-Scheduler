# MQSS Slurm SPANK Docker Testbed

A self-contained Ubuntu 24.04 image with a single-node Slurm cluster (`slurmctld` +
`slurmd`) and the `mqss-spank-plugin` built and registered in `plugstack.conf`. Used to
exercise the SPANK plugin's hooks end to end via `srun`/`sbatch`, without needing a real
MQSS token or endpoint.

Sibling of [`iqm-spank-plugin/`](../../iqm-spank-plugin/) — same mechanics (config
precedence, Slurm-license alignment check), rebranded `MQSS_*`/`mqss_*` instead of
`IQM_*`/`iqm_*`, plus a `Gres=qpu:<alias>:<n>` node declaration so `sinfo`/`scontrol show
node` report QPU availability per node (informational only for now — Slurm auto-enforces
Gres capacity on its own; the plugin does not currently cross-check `--gres=qpu:...`
requests the way it does Slurm licenses).

## Layout

```
mqss-spank-plugin/
├── CMakeLists.txt        # self-contained: cmake_minimum_required/project/option all here
├── mqss-spank-plugin.cpp
├── mqss-spank.conf.in
├── noxfile.py
├── Readme.md              (this file)
├── docker-testbed/
│   ├── Dockerfile
│   ├── create_container.sh  # build + run detached + wait ready + attach a shell
│   └── remove_container.sh  # stop the container and remove the image
├── slurm-config/
│   ├── slurm.conf          # single node "localhost", partition "mqss", Gres=qpu
│   ├── cgroup.conf
│   └── gres.conf           # declares the qpu Gres type (iqm_eqe1, iqm_qexa, aqt, qlm, ...)
└── test/
    ├── entrypoint.sh              # starts munge/slurmctld/slurmd, waits for node ready
    ├── run_docker_test.sh         # default CMD: runs the smoke test suite
    ├── test_hook_logs.sh
    ├── test_env_injection.sh
    ├── test_license_alignment.sh
    ├── submit_mqss_job.sbatch          # stub payload: prints injected env + confirms license/gres
    ├── submit_mqss_job_offload.sbatch  # real gRPC offload to ../../examples/grpc-server-example/
    ├── bell_state_offload.py           # --provider {aer,mqss-grpc} adapter pattern
    ├── quantum_job.proto               # gRPC contract (copy of ../../examples/grpc-server-example/quantum_job.proto)
    ├── quantum_job_pb2.py               # generated — see that file's Readme for regeneration
    └── quantum_job_pb2_grpc.py          # generated
```

This whole folder is the Docker build context — the Dockerfile's `COPY . /workspace`
copies it flat into `/workspace`, so `noxfile.py` and `test/` end up co-located there,
same as they are here.

## Prerequisites

- Docker Desktop.
- On Apple Silicon Macs: the base image is pinned to `--platform=linux/amd64` since
  `slurm-wlm`/`libslurm-dev` are validated for x86_64. Docker Desktop emulates this via
  QEMU automatically. For better performance, enable **Settings → General → "Use
  Rosetta for x86_64/amd64 emulation on Apple Silicon"**.

## Build the image

Run from `mqss-spank-plugin/` itself (the parent of `docker-testbed/`):

```bash
cd slurm-spank-plugins/mqss-spank-plugin
docker build -f docker-testbed/Dockerfile -t mqss-spank-testbed .
```

This bakes the plugin into the image (built once at image-build time) and installs it
into Slurm's plugin directory with a placeholder `mqss_base_url=http://mock-mqss.invalid`
in `/etc/slurm/plugstack.conf.d/mqss-spank.conf` — enough to satisfy the plugin's
"`MQSS_BASE_URL` must be set" check without needing a real MQSS token.

## Run the container

### Default: run the test suite and exit

```bash
docker run --rm mqss-spank-testbed
```

This starts munge/slurmctld/slurmd via `test/entrypoint.sh`, waits for the node to
report `idle`/`alloc`, then runs `test/run_docker_test.sh` (the smoke tests) as `CMD`.

### Interactive: keep a shell open for manual testing

`docker-testbed/create_container.sh` builds the image, runs it detached with a
bind mount (see below), waits for the Slurm node to become ready, and drops you into an
attached shell — `docker-testbed/remove_container.sh` tears it down again:

```bash
docker-testbed/create_container.sh
```

Don't do this by overriding `--entrypoint /bin/bash` directly — that skips
`entrypoint.sh` entirely, so munge/slurmctld/slurmd never start and `sinfo` fails with
`Unable to contact slurm controller (connect failure)`. If you do end up in a shell
without the services running (e.g. after `docker run --entrypoint /bin/bash ...`), start
them manually first:

```bash
/workspace/test/entrypoint.sh sleep infinity &
```

Once the node is ready, check the Gres declaration and drive `srun`/`sbatch` manually:

```bash
sinfo -o "%N %G" # NODELIST GRES — shows qpu:iqm_eqe1:1,qpu:iqm_qexa:1,qpu:aqt:1,qpu:qlm:1,...
scontrol show node localhost | grep Gres

srun --partition=mqss --mqss-qc-alias=iqm_eqe1 --licenses=mqss_iqm_eqe1:1 /bin/env | grep MQSS_
```

### Bind-mount your working copy for iterative development

Bind-mounting `mqss-spank-plugin/` over `/workspace` lets you edit the plugin source
locally and have `entrypoint.sh` detect the mount (via the missing `.built-in-image`
sentinel) and rebuild/reinstall the plugin automatically on container start, instead of
rebuilding the whole image:

```bash
docker run --rm -it \
  -v "$(pwd)":/workspace \
  mqss-spank-testbed
```

## Offloading a circuit to a remote MQSS-Scheduler (gRPC)

`test/bell_state_offload.py` has two providers: `--provider aer` runs locally (no
network, same as `bell_state_simulator.py`); `--provider mqss-grpc` serializes the
circuit and offloads it via gRPC to a separate container — see
[`examples/grpc-server-example/`](../../examples/grpc-server-example/) for that server
and why gRPC instead of the production RabbitMQ path.

```bash
# 1. Start the scheduler-side listener first:
../../examples/grpc-server-example/docker-testbed/create_container.sh

# 2. Start this container (joins the same mqss-net Docker network automatically):
docker-testbed/create_container.sh

# 3. From inside the shell it drops you into:
sbatch test/submit_mqss_job_offload.sbatch
```

`submit_mqss_job_offload.sbatch` overrides `--mqss-base-url=http://grpc-server-example:50051`
(the plugstack default is the non-resolving `http://mock-mqss.invalid`) — the client
derives the gRPC `host:port` target directly from `MQSS_BASE_URL`, so no separate
config surface was introduced for this. Expect real measurement counts in the job's
`.out` file, computed by Aer running inside the *other* container.

### Targeting the real MQSS-Scheduler pipeline instead of the stub

There is a second, real target:
[`examples/grpc-qcscheduler/`](../../examples/grpc-qcscheduler/) (built inside
`mqss_scheduler_dev`; its source depends on the sibling `scheduler` repo's build tree for
now, see that example's Readme). Unlike `grpc-server-example/`'s Python/Aer stub, it
routes through the actual, unmodified `Scheduler` → `Submitter` → QDMI pipeline. It's
fire-and-forget (accepts the job, no results round trip yet), so
`bell_state_offload.py` prints `ACCEPTED: ...` instead of `PASS`/real counts when
talking to it — that's expected, not a failure.

This example only builds/runs inside `mqss_scheduler_dev` today, not the Ubuntu-based
`qc-container` from [`../../dev-container/`](../../dev-container/)'s `docker compose`
setup (that pair is named `qc-container`/`hpc-container` specifically so it can coexist
with, rather than replace, `mqss_scheduler_dev` — but `qc-container` doesn't have the
LLVM/QDMI/submitter/scheduler stack built yet, see that folder's Known gaps). So for now,
join `mqss_scheduler_dev` to `mqss-net` manually alongside this container
(`docker network connect mqss-net mqss_scheduler_dev`), then build and start the listener
inside it (see that example's Readme for the exact `make run` invocation — plain `mpirun`
will crash once a second network interface exists; `make run` has the fix). From this
container:

```bash
sbatch --mqss-base-url=http://mqss_scheduler_dev:50051 test/submit_mqss_job_offload.sbatch
```

## Known gaps

- `test/submit_mqss_job.sbatch` remains a stub (prints injected env, confirms
  license/gres, dispatches nowhere) for when the scheduler-side listener isn't
  running; `submit_mqss_job_offload.sbatch` is the one that actually offloads.
- The gRPC offload path has no auth on the channel yet — `MQSS_TOKEN`/
  `MQSS_TOKENS_FILE` are injected into the job's environment but not currently
  forwarded to or checked by the server (see `grpc-server-example`'s Known gaps).
- The Gres declaration is informational only; the plugin does not yet cross-validate
  `--gres=qpu:...` requests against `MQSS_QC_ALIAS` the way it does Slurm licenses.
