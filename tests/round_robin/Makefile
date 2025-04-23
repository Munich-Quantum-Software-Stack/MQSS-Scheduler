# compiler
CXX=g++

# compiling, linking, and header flags
CXX_FLAGS= -g -Wall -std=c++17
LIB_RR= -lround_robin -L/home/admin/shared/scheduler/build -I/home/admin/shared/scheduler/include/round_robin
LIB_SQ= -lqueue -L/home/admin/shared/scheduler/build -I/home/admin/shared/scheduler/include/queue
LIB_QDMI= -I/home/admin/shared/QDMI/install/include/qdmi

# dependencies
SETUP_SRC=/home/admin/shared/scheduler/tests/setup/setup.cpp
SETUP_INC=-I/home/admin/shared/scheduler/include/setup

# compiling targets
TARGET=rr_test

# compile the tests
all: $(TARGET)

$(TARGET): schedulerRoundRobinTest.cpp $(SETUP_SRC)
	$(CXX) $(CXX_FLAGS) $(LIB_RR) $(LIB_QDMI) -o $(TARGET) schedulerRoundRobinTest.cpp $(SETUP) $(SETUP_INC)

clean:
	rm -rf $(TARGET)
