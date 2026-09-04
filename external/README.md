# External Dependencies

This directory manages the MQSS packages that examples like
[`../examples/qrm-sscheduler/`](../examples/qrm-sscheduler/) link against. The `Scheduler`
library itself doesn't need any of this — see the top-level [README.md](../README.md#architecture).

## Structure

```
external/
├── clone.sh                    # Clone QDMI/QInfo/submitter sources
├── build.sh                    # Build and install all dependencies into installed/
├── QDMI-Device-Example/        # Bundled C++ QDMI binding (not cloned, lives here)
│   ├── CMakeLists.txt
│   ├── include/
│   └── src/
├── QDMI/                       # Cloned by clone.sh
├── QInfo/                      # Cloned by clone.sh
├── submitter/                  # Cloned by clone.sh
├── selector/                   # Python-based device/pass selector (separate subproject)
└── installed/                  # Final install prefix: include/, lib/ (created by build.sh)
```

`clone.sh`/`build.sh` are the source of truth for exactly which ref of each package is
pinned and why — see their own comments rather than duplicating that here.

## Quick Start

```bash
# 1. Clone all sources
bash clone.sh

# 2. Build and install (uses all CPU cores by default)
bash build.sh

# Optionally override LLVM location or parallelism:
LLVM_ROOT=/path/to/llvm bash build.sh --jobs 8
```

To refresh sources to the latest commits on their tracked branches:

```bash
bash clone.sh --update
```

## Notes

**QDMI-Device-Example** is the local C++ QDMI binding (adapted from the internal CxxQDMI package). It is built directly from source inside this folder — no cloning needed.

**LLVM** is not built here. It is pre-built and lives in the top-level or somewhere you can specify.

## Build Order

The packages must be built in this order (each depends on the previous):

```
QDMI → QDMI-Device-Example → QInfo → submitter
```

`build.sh` handles this automatically.

## After Building

The scheduler's CMake `Find*.cmake` modules look for headers and libraries under `external/installed/`. Once the build is done, configure the scheduler normally:

```bash
cd ../build
cmake ..
make -j$(nproc)
```
