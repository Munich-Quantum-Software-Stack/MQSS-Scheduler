WORKDIR=/home/admin/shared

export CPATH=$WORKDIR/submitter/install/include:$WORKDIR/QDMI/install/include:$WORKDIR/QInfo/install/include:$WORKDIR/scheduler/install/include:$WORKDIR/CxxQDMI/install/include
export INCUDE=$WORKDIR/submitter/install/include:$WORKDIR/QDMI/install/include:$WORKDIR/QInfo/install/include:$WORKDIR/scheduler/install/include:$WORKDIR/CxxQDMI/install/include
export LD_LIBRARY_PATH=$WORKDIR/submitter/install/lib:$WORKDIR/QInfo/install/lib:$WORKDIR/QDMI/build/examples/device/cxx:$WORKDIR/QDMI/build/examples/driver:/usr/lib/llvm-18/lib:usr/local/lib:$WORKDIR/scheduler/build:$WORKDIR/CxxQDMI/install/lib
export LIBRARY_PATH=$WORKDIR/submitter/install/lib:$WORKDIR/QInfo/install/lib:$WORKDIR/QDMI/build/examples/device/cxx:$WORKDIR/QDMI/build/examples/driver:/usr/lib/llvm-18/lib:/usr/local/lib:$WORKDIR/scheduler/build:$WORKDIR/CxxQDMI/install/lib
export QDMI_CONF=$WORKDIR/CxxQDMI/build/tests/qdmi.conf

