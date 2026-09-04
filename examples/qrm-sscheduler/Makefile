WORKDIR=/home/admin/shared/MQSS-Scheduler
LLVM_ROOT=/home/admin/shared/dependencies/installed

CXX=g++
MPICXX=/usr/bin/mpicxx
MPIRUN=/usr/bin/mpirun
PROTOC=protoc
GRPC_CPP_PLUGIN=$(shell which grpc_cpp_plugin)
CXXFLAGS=-O3 -std=c++20 -g
GRPC_FLAGS=$(shell pkg-config --cflags --libs grpc++ protobuf)
LDFLAGS=-L$(LLVM_ROOT)/lib -lLLVM-16 \
		-lqinfo -L$(WORKDIR)/external/installed/lib \
		-lqdmi_cpp -L$(WORKDIR)/external/installed/lib \
		-lsubmitter -L$(WORKDIR)/external/installed/lib \
		-lscheduler -L$(WORKDIR)/install/lib \
		-Wl,-rpath,$(WORKDIR)/install/lib \
		$(GRPC_FLAGS)
INCFLAGS=-I$(LLVM_ROOT)/include \
		-I$(WORKDIR)/external/installed/include \
		-I$(WORKDIR)/include \
		-I$(WORKDIR)/install/include \
		-I.

PROTO_GEN=quantum_job.pb.cc quantum_job.pb.h quantum_job.grpc.pb.cc quantum_job.grpc.pb.h
INPUT=qrm_sscheduler.cpp mqss_grpc_server.cpp quantum_job.pb.cc quantum_job.grpc.pb.cc
OUTPUT=qrm_sscheduler

all: $(OUTPUT)

# Not committed — the generated bindings are tied to the exact protobuf/grpc
# version installed in whichever container builds this - regenerate rather
# than reuse a copy from elsewhere (protobuf version mismatches between
# differently-built stubs are a real, previously-hit failure mode).
proto: $(PROTO_GEN)

$(PROTO_GEN): quantum_job.proto
	$(PROTOC) -I. --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) quantum_job.proto

$(OUTPUT): $(INPUT)
	$(MPICXX) $(INPUT) -o $(OUTPUT) $(CXXFLAGS) $(LDFLAGS) $(INCFLAGS)

# --mca pml ob1 --mca btl self,vader --mca mtl ^ofi: both ranks run on this
# one node, so force shared-memory-only MPI transport (avoids OpenMPI's
# libfabric/OFI layer probing every network interface at MPI_Init time).
#
# --mca odls_base_sigkill_timeout 8: give the graceful-shutdown cascade
# (rank 0's gRPC Shutdown() -> shutdown sentinel -> rank 1's submitter
# thread stopping) room to finish before mpirun SIGKILLs the ranks (default
# timeout: 1s, too short). See qrm_sscheduler.cpp's top-of-file
# shutdown-handling comment for a known gap in how reliably this cascade
# currently triggers.
#
# Only 2 ranks: rank 0 is the gRPC listener, rank 1 runs Scheduler with the
# QDMI Submitter embedded as an in-process thread rather than farmed out to
# further MPI ranks — see qrm_sscheduler.cpp's top-of-file comment and this
# directory's Readme for why.
run:
	$(MPIRUN) --mca pml ob1 --mca btl self,vader --mca mtl ^ofi \
		--mca odls_base_sigkill_timeout 8 -np 2 ./$(OUTPUT)

clean:
	rm -rf $(OUTPUT) $(PROTO_GEN)
