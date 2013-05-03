#!/bin/bash

PORT=50770
SOCKET=/tmp/nena_socket_client

DEMO_PATH=../../../demoApps

cd $DEMO_PATH/tmnet
python3 webapi.py --port=$PORT --socket=$SOCKET
