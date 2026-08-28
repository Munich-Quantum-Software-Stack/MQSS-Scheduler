# dev-container

`docker compose` setup that starts a second, Ubuntu-based dev environment + Slurm SPANK
testbed pair, already on the same Docker network (`mqss-net`) — no manual
`docker network connect` step needed between them.

Named `qc-container` (MQSS-Scheduler side) and `hpc-container` (Slurm side) specifically
so they **coexist alongside**, rather than replace, the pre-existing manually-created
`mqss_scheduler_dev` / `mqss-slurm-testbed` containers — those are Rocky Linux 9.3 and
already have the full LLVM/QDMI/QInfo/submitter/scheduler dependency stack built; this
compose pair is Ubuntu 24.04 and starts from a clean slate. Both pairs can run at the
same time, on the same `mqss-net` network, distinguished by container name/hostname.

## Services

| Service | Container name | What it is |
|---|---|---|
| `qc-container` | `qc-container` | MQSS-Scheduler dev environment — bind-mounts just this repo at `/home/admin/shared/MQSS-Scheduler` (deliberately *not* the whole `~/Projects/mqss` workspace like `mqss_scheduler_dev` does — that was making Tab-completion and other directory scans crawl/hang across hundreds of thousands of sibling-repo files; everything this container's build needs is vendored under `external/` in this repo already). Builds/binaries (e.g. [`../examples/grpc-qcscheduler/`](../examples/grpc-qcscheduler/)) still need to be built manually inside it — this only provisions the OS/toolchain (see `Dockerfile`), and it does **not** yet have the LLVM/QDMI/submitter/scheduler stack built (see Known gaps). |
| `hpc-container` | `hpc-container` | [`../slurm-spank-plugins/mqss-spank-plugin/`](../slurm-spank-plugins/mqss-spank-plugin/)'s Slurm + SPANK plugin testbed. |

Just these two — for the lightweight Python/Aer stub listener instead of the real
pipeline, use [`../examples/grpc-server-example/`](../examples/grpc-server-example/)'s own
`docker-testbed/create_container.sh` standalone (it joins the same `mqss-net` network).

## Usage

```bash
# Start both containers, already on mqss-net:
docker compose up -d --build

# Attach a shell to either:
docker compose exec qc-container bash
docker compose exec hpc-container bash

# From hpc-container, submit a job that offloads to qc-container by container name:
sbatch --mqss-base-url=http://qc-container:50051 \
  test/submit_mqss_job_offload.sbatch

# Tear down (only these two — mqss_scheduler_dev/mqss-slurm-testbed are untouched):
docker compose down
```

`hpc-container`'s `entrypoint.sh` still starts munge/slurmctld/slurmd on container start
and detects the bind mount to rebuild the SPANK plugin automatically, exactly as
`../slurm-spank-plugins/mqss-spank-plugin/docker-testbed/create_container.sh` already
does — this compose file just orchestrates the same image/mounts under a different
container name, alongside `qc-container`, instead of running it standalone.

## Relationship to mqss_scheduler_dev / mqss-slurm-testbed

No conflict and no cleanup needed — `container_name`/`hostname` differ, so
`docker compose up` won't collide with them. `mqss-net` is declared `external: true` in
`docker-compose.yml` specifically so compose joins the existing network (already home to
`mqss_scheduler_dev`/`mqss-slurm-testbed`) as-is rather than asserting ownership of it —
without that, compose refuses to start with a "network ... has incorrect label" error the
moment the network already exists and wasn't created by compose itself, which it usually
won't have been. If `mqss-net` genuinely doesn't exist yet on a fresh machine, create it
once first (`docker network create mqss-net`) — `external: true` means compose expects it
to exist rather than creating it.

## Known gaps

- `qc-container`'s image only provisions the OS/toolchain (grpc, protobuf, openmpi,
  build tools) — QDMI/QInfo/submitter still need building inside it via this repo's own
  `external/clone.sh`+`external/build.sh` (already done for QDMI and
  `QDMI-Device-Example` as of this writing; QInfo/submitter not yet).
- **`qc-container` is Ubuntu 24.04, `mqss_scheduler_dev` is Rocky Linux 9.3** (see
  `qc-dockerfile/Dockerfile`'s top comment) — but this turned out not to be the
  compatibility problem it looked like. LLVM at `~/Projects/mqss/dependencies/installed`
  is Rocky-built and is mounted read-only into `qc-container` anyway (see
  `docker-compose.yml`) — verified working (compiled, linked, and ran a real test program
  against it) despite the distro difference: glibc/libstdc++ are backward compatible in
  this direction (older-distro binary, newer runtime), so no Ubuntu-native LLVM rebuild
  was needed after all. `scheduler/build`/`scheduler/dependencies/installed` (also
  Rocky-built) haven't specifically been tested the same way — likely fine by the same
  reasoning, but unconfirmed. Either way,
  [`../examples/grpc-qcscheduler/`](../examples/grpc-qcscheduler/) (including the
  real-execution work — see that example's Readme) still only actually runs in
  `mqss_scheduler_dev` today, since it needs QDMI/QInfo/submitter/scheduler built inside
  whichever container it runs in, and that hasn't been done end-to-end in `qc-container`
  yet (see the point above).
- No healthcheck/dependency ordering between the two services — `hpc-container` doesn't
  wait for `qc-container`'s `grpc_qcscheduler` to actually be listening (nothing starts
  it automatically either; that's still a manual `make run` inside whichever container
  you're using).
