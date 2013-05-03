#!/bin/bash

REMOTENETLETS=10.10.10.4:8080/nodearch-available-netlets
#REMOTENETLETS=localhost:8080/nodearch-available-netlets
LOCALNETLETS=/tmp/nodearch-available-netlets

wget $REMOTENETLETS -q -O $LOCALNETLETS

