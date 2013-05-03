#!/bin/bash

LWHOST="localhost:50770"

if [ $# -ne 1 ]; then
	echo "Usage: $0 <scheme>"
	exit 1
fi

LWSCHEME=$1
LWUUID=`uuid -v5 ns:URL $LWSCHEME`
LWXPI="localweb-$LWSCHEME-0.1.xpi"

echo "$LWSCHEME {$LWUUID} $LWXPI"

mkdir -p tmp/components
cat src/chrome.manifest | sed s/LWSCHEME/"$LWSCHEME"/ | sed s/LWUUID/"$LWUUID"/ >tmp/chrome.manifest
cat src/install.rdf | sed s/LWSCHEME/"$LWSCHEME"/ | sed s/LWUUID/"$LWUUID"/ >tmp/install.rdf
cat src/components/localweb.js | sed s/LWSCHEME/"$LWSCHEME"/ | sed s/LWUUID/"$LWUUID"/ | sed s/LWHOST/"$LWHOST"/ >tmp/components/localweb.js

cd tmp
zip -r ../$LWXPI *

