#!/bin/bash

LOCALFILE=nodearch-available-netlets
REMOTEFILE=nodearch-available-netlets
SERVER=10.10.10.4:8080
#SERVER=localhost:8080

wget --post-file=$LOCALFILE ${SERVER}/${REMOTEFILE} -q -O /dev/null
