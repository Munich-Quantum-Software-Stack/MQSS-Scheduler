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
| submitter                | bundled in `dependencies/submitter/`               | —                        |


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

Notes with the updated version of QDMI: Breaking API changes (develop vs v1.1.0)
1. QDMI_job_wait signature changed — hardest break

// v1.1.0
int QDMI_job_wait(QDMI_Job job);

// develop
int QDMI_job_wait(QDMI_Job job, size_t timeout);
This breaks the function pointer typedef in FakeQDMI-Device-Example/include/qdmi_lib.hpp and every caller of QDMI_job_wait in qdmi_lib.cpp and anywhere in the submitter/examples.

2. QDMI_SESSION_PARAMETER_* enum values renumbered

// v1.1.0               → develop
QDMI_SESSION_PARAMETER_USERNAME  = 1  →  3  (new AUTHFILE=1, AUTHURL=2 inserted before it)
QDMI_SESSION_PARAMETER_PROJECTID = 2  →  5  (new PASSWORD=4 inserted)
QDMI_SESSION_PARAMETER_MAX       = 3  →  6
Any code passing these as integers over a binary boundary breaks silently.

3. New QDMI_job_query_property function added
A new function + QDMI_Job_Property enum is introduced. FakeQDMI-Device-Example would need to implement and expose it.

4. QDMI_SITE_PROPERTY_T1 type changed

// v1.1.0: double t1
// develop: uint64_t t1
Any code reading T1 values with the old type assumption silently reads garbage.

What needs updating to use develop
What	Change needed
qdmi_lib.hpp	QDMI_job_wait_t typedef: add size_t timeout param
qdmi_lib.cpp	All QDMI_job_wait_ptr(job) calls → QDMI_job_wait_ptr(job, 0)
qdmi_lib.hpp + qdmi_lib.cpp	Add QDMI_job_query_property_t typedef + load_symbol + implementation
FakeQDMI-Device-Example device impl	Implement QDMI_job_query_property in the device
Any code using QDMI_SESSION_PARAMETER_USERNAME/PROJECTID	Recompile against new headers (enum values shifted)
Any code reading QDMI_SITE_PROPERTY_T1	Change buffer type from double to uint64_t
clone.sh	Change v1.1.0 → new tag or develop
The QDMI_job_wait signature change is the most impactful — it touches FakeQDMI-Device-Example, the submitter, and potentially the scheduler. It's doable but requires careful tracing of all callers.
