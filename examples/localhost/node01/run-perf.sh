#!/bin/bash

DEMO_PATH=../../../demoApps

LD_LIBRARY_PATH=$DEMO_PATH/tmnet $DEMO_PATH/perf-test/perf_tmnet --destination app://localhost.node02/perf_tmnet -i 125 $@

