# Scheduler Dependencies

This directory manages the MQSS packages that the scheduler links against.

## Structure

```
dependencies/
├── clone.sh                    # Clone external dependency sources into src/
├── build.sh                    # Build and install all dependencies into installed/
├── FakeQDMI-Device-Example/    # Bundled C++ QDMI binding (not cloned, lives here)
│   ├── CMakeLists.txt
│   ├── include/
│   └── src/
├── src/                        # Cloned source repos (created by clone.sh)
└── installed/                  # Final install prefix: include/, lib/ (created by build.sh)
```

## Dependencies

| Package                  | Source                                             | Ref                      |
|--------------------------|----------------------------------------------------|--------------------------|
| QDMI                     | Munich-Quantum-Software-Stack/QDMI                 | tag `v1.1.0`             |
| FakeQDMI-Device-Example  | bundled in `dependencies/FakeQDMI-Device-Example/` | —                        |
| QInfo                    | Munich-Quantum-Software-Stack/QInfo                | branch `develop`         |
| submitter                | Munich-Quantum-Software-Stack/submitter            | branch `minh/ssintegrat` |


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

**FakeQDMI-Device-Example** is the local C++ QDMI binding (adapted from the internal CxxQDMI package). It is built directly from source inside this folder — no cloning needed.

**LLVM** is not built here. It is pre-built and lives in the top-level or somewhere you can specify.

## Build Order

The packages must be built in this order (each depends on the previous):

```
QDMI → FakeQDMI-Device-Example → QInfo → submitter
```

`build.sh` handles this automatically.

## After Building

The scheduler's CMake `Find*.cmake` modules look for headers and libraries under `dependencies/installed/`. Once the build is done, configure the scheduler normally:

```bash
cd ../build
cmake ..
make -j$(nproc)
```
