## Examples

The MQSS quantum scheduler works as a daemon running to manage arrived quantum jobs, scheduler queue, and handing jobs to relevant submitter queues of the corresponding QDMI device before the jobs can be offloaded to the real QPU. Therefore, each example in this folder works as a stand-alone scheduler runtime.

- `job_submission`: to test the example of generating quantum jobs.
- `single-mpi-qcscheduler`: to test all components and work together, where the connection is conducted via MPI, each process launches a component, such as job creator, scheduler, submitter, etc.
- `multi-mpi-qcscheduler`: to test all components, where each is launched as an independent program and the connection is still via MPI.
