#!/bin/bash


if [ $# -ne 1 ]; then
	echo "Missing command line parameter"
	exit 1
fi

cd /home/denis/devel/nena/demoApps/mediaPlayer2012
LD_LIBRARY_PATH=../tmnet ./mediaPlayer $1 2>&1 1>/home/denis/devel/nena/examples/euroview2012/client/mediaPlayer.log

