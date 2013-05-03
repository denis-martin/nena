#!/bin/bash

DEMO_PATH=../../../demoApps

LD_LIBRARY_PATH=$DEMO_PATH/tmnet $DEMO_PATH/pingPong/ping_tmnet --destination app://localhost.node02/ping_tmnet $@

