#!/bin/bash

NODE="node://kit/videostore"

NAD="../../../build/targets/boost/nad"
GDB=

if [[ $1 == "gdb" ]]; then
	GDB="gdb --args"
fi

if [ -f "$NAD" ]; then
	# to run the daemon within GDB, just prepend "gdb --args " (without parentheses)
	$GDB $NAD --name $NODE --vvvv;
else
	echo
	echo "  Please compile the daemon and the boost target first"
	echo "  by typing \"scons && scons boost\" in the root"
	echo "  directory of the daemon source package"
	echo;
fi

