#!/bin/bash

SOCKET="/tmp/nena_socket_router"

DEMO_PATH=../../../demoApps

cd $DEMO_PATH/demoNetworkGui
python demoNetworkGui.py --socket=$SOCKET
