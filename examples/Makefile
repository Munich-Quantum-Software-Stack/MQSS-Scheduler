WORKDIR=/home/admin/shared

CXX=g++
MPICXX=/usr/lib64/openmpi/bin/mpicxx
CXXFLAGS=-O3 -std=c++17 -g
LDFLAGS=-L$(WORKDIR)/dependencies/installed/lib  -lLLVM-16 \
		-lqinfo -L$(WORKDIR)/QInfo/install/lib \
		-lqdmi_cpp -L$(WORKDIR)/CxxQDMI/install/lib -L$(WORKDIR)/QDMI/build/examples/device/cxx \
		-lsubmitter -L$(WORKDIR)/submitter/install/lib \
		-lscheduler -L$(WORKDIR)/scheduler/build
INCFLAGS=-I/usr/lib/llvm-18/include \
		-I$(WORKDIR)/QInfo/install/include \
		-I$(WORKDIR)/CxxQDMI/install/include \
		-I$(WORKDIR)/QDMI/install/include \
		-I$(WORKDIR)/submitter/install/include \
		-I$(WORKDIR)/scheduler/include

INPUT=multiprocs_mqss_scheduler.cpp 
OUTPUT=multiprocs_mqss_scheduler

all: mqss_scheduler

mqss_scheduler:
	$(MPICXX) $(INPUT) -o $(OUTPUT) $(CXXFLAGS) $(LDFLAGS) $(INCFLAGS)

run:
	mpirun -np 3 ./$(OUTPUT)

clean:
	rm -rf $(OUTPUT)
