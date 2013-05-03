#!/bin/bash

DEV1=eth0
DEV2=eth1
#DEV3=eth5
DEV3=eth2
BRIDGE=br0

/sbin/ifconfig $DEV1 up 0.0.0.0
/sbin/ifconfig $DEV2 up 0.0.0.0
/sbin/ifconfig $DEV3 up 0.0.0.0

/usr/sbin/brctl addbr $BRIDGE
/usr/sbin/brctl addif $BRIDGE $DEV1
/usr/sbin/brctl addif $BRIDGE $DEV2
/usr/sbin/brctl addif $BRIDGE $DEV3

#ifconfig $BRIDGE up 0.0.0.0
/sbin/ifconfig $BRIDGE up 10.10.10.4

/usr/sbin/brctl show
