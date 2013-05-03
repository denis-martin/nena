#!/bin/bash

NAD="../../../build/targets/omnetpp/omnet_na"

if [ -f "$NAD" ]; then
	gdb --args $NAD -n ..;
else
	echo
	echo "  Please compile the daemon and the OMNeT target first"
	echo "  by typing \"scons && scons omnet\" in the root"
	echo "  directory of the daemon source package"
	echo;
fi

