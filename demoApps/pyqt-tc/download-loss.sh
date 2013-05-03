#!/bin/bash

REMOTELOSS=10.10.10.4:8080/nodearch-pkt-loss
#REMOTELOSS=localhost:8080/nodearch-pkt-loss
LOCALLOSS=/tmp/nodearch-pkt-loss

while [ true ]; do
	wget $REMOTELOSS -q -O $LOCALLOSS
	
	sleep 1
done
