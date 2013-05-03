#!/bin/bash

DIR="/home/glab/nena/demoApps/videoStore2012/videos/"
URI="nenacdn://kit.videostore/videos"
SOCKET="/tmp/nena_socket_videostore"

DEMO_PATH=../../../demoApps

cd $DEMO_PATH/fileServer
python fileServer.py --dir=$DIR --uri=$URI --socket=$SOCKET
