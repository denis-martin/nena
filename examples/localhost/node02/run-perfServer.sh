#!/bin/bash

DEMO_PATH=../../../demoApps

LD_LIBRARY_PATH=$DEMO_PATH/tmnet $DEMO_PATH/perf-test/perf_tmnet -s -p /tmp/nena_socket_node02

