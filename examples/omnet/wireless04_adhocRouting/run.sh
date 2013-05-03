#!/bin/bash

# uncomment this line to run the simulator within gdb
GDB="gdb --args"

OPPPATH=`cat \`opp_configfilepath\` | grep "OMNETPP_ROOT = " | sed "s/OMNETPP_ROOT = //"`
MF2PATH="$OPPPATH/mf2";
NAD="../../../build/targets/omnetpp/omnet_na"

if [ ! -d $OPPPATH ]; then
	echo
	echo "  Path to OMNeT++ not found. Please check if the command \"opp_configfilepath\""
	echo "  is in your \$PATH."
	echo
	exit;
fi

if [ ! -d $MF2PATH ]; then
	echo
	echo "  Path to MF2 invalid: $MF2PATH"
	echo "  Please modify the variable \$MF2PATH in this script"
	echo "  according to your installation"
	echo
	exit;
fi

if [ ! -f "$MF2PATH/core/libmfcore.so" ] || [ ! -f "$MF2PATH/contrib/libmfcontrib.so" ]; then
	echo
	echo "  $MF2PATH/src/libmfcore.so and/or $MF2PATH/src/libmfcontrib.so not found."
	echo "  Please compile the MF2 core and contrib libraries first."
	echo
	exit;
fi

if [ -f "$NAD" ]; then
	echo "Running LD_LIBRARY_PATH=\"$MF2PATH/core:$MF2PATH/contrib\" $GDB $NAD -l $MF2PATH/core/mfcore -l $MF2PATH/contrib/mfcontrib -n \"..:$MF2PATH/src\""
	LD_LIBRARY_PATH="$MF2PATH/core:$MF2PATH/contrib" $GDB $NAD -l $MF2PATH/core/mfcore -l $MF2PATH/contrib/mfcontrib -n "..:$MF2PATH/core:$MF2PATH/contrib";
else
	echo
	echo "  Please compile the daemon and the OMNeT target first"
	echo "  by typing \"scons && scons mf2=1 omnet\" in the root"
	echo "  directory of the daemon source package"
	echo;
fi

