#!/bin/bash

SCRIPTDIR=.
NADDIR=../../build/targets/boost
IF=eth0

# enough command line parameters?
if [ -z $1 ]; then
	echo "Usage: $0 <num>"
	echo "num: Number of node"
	exit
fi

# configure interface

sudo ifconfig $IF up 10.10.10.$1 netmask 255.255.255.0

# set buffer sizes
sudo sysctl -w net.core.rmem_max=2097152
sudo sysctl -w net.core.wmem_max=1048576

# (re)start background downloader TODO: stop it somehow after script termination
cd $SCRIPTDIR

killall download-loss.sh
./download-loss.sh &

# start node architecture daemon
cd $NADDIR

./nad --port 50779 --vvv --unlink boost_sock --name node://tm.uka.de/nodearch/cam$1

