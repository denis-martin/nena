#!/bin/bash

IF=eth1
IP=10.10.10.9
NETMASK=255.255.255.0
#GATEWAY=10.10.10.4
NAMESERVER=141.3.70.3

sudo ifconfig $IF up $IP netmask $NETMASK
#sudo route add default gw $GATEWAY
#echo nameserver $NAMESERVER | sudo tee /etc/resolv.conf

# hard coded settings for routing/NAT
echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
