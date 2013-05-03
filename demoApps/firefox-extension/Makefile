# Change this to point at your Gecko SDK directory.
GECKO_SDK_PATH ?= $(HOME)/devel/xulrunner-sdk


VERSION = 0.1.$(shell date "+%Y%m%d")

GECKO_VERSION = $(shell grep "Milestone" ${GECKO_SDK_PATH}/bin/platform.ini | cut -d= -f2 )

OS = Linux

ARCH = $(shell uname -m |sed 's/i686/x86/')

TARGET_COMPILER_ABI = gcc3

ABI := ${OS}_${ARCH}-${TARGET_COMPILER_ABI}

XPI_FILE := nenaprotocols-${VERSION}-ff${GECKO_VERSION}-${ABI}.xpi


all: ${XPI_FILE}

deploy: ${XPI_FILE}
	# run seperate firefox for debugging
	firefox -no-remote -P dev ${XPI_FILE}

components/nenaprotocols.so:
	make -C components/src/ GECKO_SDK_PATH=${GECKO_SDK_PATH}
	cp components/src/nenaprotocols.so components

components/INenaProtocolHandler.xpt:
	cp components/src/INenaProtocolHandler.xpt components

install.rdf: install.rdf.in
	cat install.rdf.in | sed 's/HERE_VERSION/${VERSION}/' | sed 's/HERE_FFVERSION/${GECKO_VERSION}/' | sed 's/HERE_ABI/${ABI}/' >install.rdf

${XPI_FILE}: install.rdf chrome.manifest components/nenaprotocols.so components/INenaProtocolHandler.xpt
	zip ${XPI_FILE} install.rdf chrome.manifest components/nenaprotocols.so components/INenaProtocolHandler.xpt

clean:
	rm -f ${XPI_FILE} install.rdf components/nenaprotocols.so components/INenaProtocolHandler.xpt
	make -C components/src/ clean
