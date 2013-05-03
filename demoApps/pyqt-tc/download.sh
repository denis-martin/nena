#!/bin/bash

#REMOTENETLETS=10.10.10.4:8080/nodearch-available-netlets
REMOTENETLETS=localhost:8080/nodearch-available-netlets
LOCALNETLETS=nodearch-available-netlets
#REMOTELOSS=10.10.10.4:8080/nodearch-pkt-loss
REMOTELOSS=localhost:8080/nodearch-pkt-loss
LOCALLOSS=nodearch-pkt-loss

while [ true ]; do
	wget $REMOTENETLETS -q -O $LOCALNETLETS  
	wget $REMOTELOSS -q -O $LOCALLOSS
	
	sleep 1
done
