#!/bin/bash

DEV1=eth0
DEV2=eth1
#DEV3=eth5
DEV3=eth2
BRIDGE=br0

ifconfig $DEV1 up 0.0.0.0
ifconfig $DEV2 up 0.0.0.0
ifconfig $DEV3 up 0.0.0.0

brctl addbr $BRIDGE
brctl addif $BRIDGE $DEV1
brctl addif $BRIDGE $DEV2
brctl addif $BRIDGE $DEV3

#ifconfig $BRIDGE up 0.0.0.0
ifconfig $BRIDGE up 10.10.10.4

brctl show
