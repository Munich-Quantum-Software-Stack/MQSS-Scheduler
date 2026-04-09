WORKDIR=/home/admin/shared

QDMI_INC=$WORKDIR/QDMI/install/include
QDMI_LIB=$WORKDIR/QDMI/install/lib
QDMI_EXAMPLE_DEV=$WORKDIR/QDMI/build/examples/device/cxx
QDMI_FCXX_DEV=$WORKDIR/CxxQDMI/build/src

QINFO_INC=$WORKDIR/QInfo/install/include
QINFO_LIB=$WORKDIR/QInfo/install/lib

SUBMITTER_INC=$WORKDIR/submitter/install/include
SUBMITTER_LIB=$WORKDIR/submitter/install/lib

SCHEDULER_INC=$WORKDIR/scheduler/install/include
SCHEDULER_LIB=$WORKDIR/scheduler/install/lib

LLVM_INC=$WORKDIR/dependencies/installed/include
LLVM_LIB=$WORKDIR/dependencies/installed/lib

export LLVM_ROOT=$WORKDIR/dependencies/installed
export QDMI_ROOT=$WORKDIR/QDMI/install
export CXX_QDMI_ROOT=$WORKDIR/CxxQDMI/install
export QDMI_CONF=$WORKDIR/CxxQDMI/build/tests/qdmi.conf
export SPDLOG_INC=$WORKDIR/scheduler/build/_deps/spdlog-src/include
export SPDLOG_LIB=$WORKDIR/shared/scheduler/build/_deps/spdlog-build

export CPATH=$SUBMITTER_INC:$QDMI_INC:$QINFO_INC:$LLVM_INC:$SPDLOG_INC:$SCHEDULER_INC:$CPATH
export INCLUDE=$SUBMITTER_INC:$QDMI_INC:$QINFO_INC:$LLVM_INC:$SPDLOG_INC:$SCHEDULER_INC:$INCLUDE
export LD_LIBRARY_PATH=$SUBMITTER_LIB:$QINFO_LIB:$LLVM_LIB:$QDMI_EXAMPLE_DEV:$QINFO_LIB:$QDMI_FCXX_DEV:$SPDLOG_LIB:/usr/local/lib:$SCHEDULER_LIB:$LD_LIBRARY_PATH
export LIBRARY_PATH=$SUBMITTER_LIB:$QINFO_LIB:$LLVM_LIB:$QDMI_EXAMPLE_DEV:$QINFO_LIB:$QDMI_FCXX_DEV:$SPDLOG_LIB:/usr/local/lib:$LIBRARY_PATH
export PATH=/usr/lib64/openmpi/bin:$PATH
