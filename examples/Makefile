WORKDIR=/home/admin/shared

CXX=g++
CXXFLAGS=-O3 -std=c++17 -g
LDFLAGS=-L/usr/lib/llvm-18/lib -lLLVM-18 \
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

INPUT=qcjob_submission.cpp
OUTPUT=qcjob_submission

all: qcjob_submit

qcjob_submit:
	$(CXX) $(INPUT) -o $(OUTPUT) $(CXXFLAGS) $(LDFLAGS) $(INCFLAGS)

run:
	./$(OUTPUT)

clean:
	rm -rf $(OUTPUT)
