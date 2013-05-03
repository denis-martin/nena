#!/bin/bash

DEMO_PATH=../../../demoApps

LD_LIBRARY_PATH=$DEMO_PATH/tmnet $DEMO_PATH/pingPong/ping_tmnet -s -p /tmp/nena_socket_node02

