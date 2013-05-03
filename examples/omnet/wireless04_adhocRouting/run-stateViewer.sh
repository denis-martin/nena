#!/bin/bash

SV="../../../demoApps/stateViewer/stateViewer"

if [ -f "$SV" ]; then
	$SV -c
else
	echo
	echo "  Please compile the stateViewer application first"
	echo "  by typing \"qmake-qt4 && make\" in the directory"
	echo "  demoApps/stateViewer."
	echo;
fi

