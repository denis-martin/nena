#!/bin/bash

IF=eth0
IP=10.10.10.1
NETMASK=255.255.255.0
GATEWAY=10.10.10.9
NAMESERVER=141.3.70.3

sudo ifconfig $IF up $IP netmask $NETMASK
sudo route add default gw $GATEWAY
echo nameserver $NAMESERVER | sudo tee /etc/resolv.conf

