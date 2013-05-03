#!/bin/bash

NAD="../../../build/targets/boost/nad"
GDB=
TASKSET=

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../../../build/buildingBlocks/edu.kit.tm/itm/sig
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../../../build/buildingBlocks/edu.kit.tm/itm/transport
export LD_LIBRARY_PATH

if [[ $1 == "gdb" ]]; then
        GDB="gdb --args"
fi

if [[ $1 == "taskset" ]]; then
    TASKSET="taskset -a -c 1"
fi

if [ -f "$NAD" ]; then
	# to run the daemon within GDB, just prepend "gdb --args " (without parentheses)
	$GDB $TASKSET $NAD --name node://localhost/node02 --port 50780 --vvvv;
else
	echo
	echo "  Please compile the daemon and the boost target first"
	echo "  by typing \"scons && scons boost\" in the root"
	echo "  directory of the daemon source package"
	echo;
fi

