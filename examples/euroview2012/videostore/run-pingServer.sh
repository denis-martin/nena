#!/bin/bash

DEMO_PATH=../../../demoApps

LD_LIBRARY_PATH=$DEMO_PATH/tmnet $DEMO_PATH/pingPong/ping_tmnet -s -d nenaweb://kit.videostore/ping_tmnet -p /tmp/nena_socket_videostore

