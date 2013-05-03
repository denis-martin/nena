#!/bin/bash

# uncomment this line to run the simulator within gdb
GDB="gdb --args"

OPPPATH=`cat \`opp_configfilepath\` | grep "OMNETPP_ROOT = " | sed "s/OMNETPP_ROOT = //"`
INETPATH="$OPPPATH/inetmanet"
NAD="../../../build/targets/omnetpp/omnet_na"

if [ ! -d $OPPPATH ]; then
	echo
	echo "  Path to OMNeT++ not found. Please check if the command \"opp_configfilepath\""
	echo "  is in your \$PATH."
	echo
	exit;
fi

if [ ! -d $INETPATH ]; then
        echo
	echo "  Path to INETMANET fork invalid: $INETPATH"
	echo "  Please modify the variable \$INETPATH in this script"
	echo "  according to your installation"
        echo
        exit;
fi

if [ ! -f $INETPATH/src/libinet.so ]; then
	echo
	echo "  $INETPATH/src/libinet.so not found."
	echo "  Please compile the INETMANET fork first."
	echo
	exit;
fi

if [ -f "$NAD" ]; then
	echo "Running LD_LIBRARY_PATH=$INETPATH/src $GDB $NAD -l $INETPATH/src/inet -n \"..:$INETPATH/src\""
	LD_LIBRARY_PATH=$INETPATH/src:$OPPPATH/lib $GDB $NAD -l $INETPATH/src/inet -n "..:$INETPATH/src";
else
	echo
	echo "  Please compile the daemon and the OMNeT target first"
	echo "  by typing \"scons && scons inet=1 omnet\" in the root"
	echo "  directory of the daemon source package"
	echo;
fi

