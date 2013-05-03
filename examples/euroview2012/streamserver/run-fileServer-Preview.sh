#!/bin/bash

DIR="/home/glab/nena/demoApps/ev2012videostore/videos/"
URI="nenavideo://kit.streamserver/videos"
SOCKET="/tmp/nena_socket_streamserver"

DEMO_PATH=../../../demoApps

cd $DEMO_PATH/fileServer
python fileServer.py --dir=$DIR --uri=$URI --socket=$SOCKET
