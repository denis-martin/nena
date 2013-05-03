#!/bin/bash



../../../demoApps/mscgen/msglog2signalling.pl nena-messages-send.log
../../../demoApps/mscgen/msglog2signalling.pl nena-messages-recv.log

~/devel/msc-generator/src/msc-gen nena-messages-send.signalling
~/devel/msc-generator/src/msc-gen nena-messages-recv.signalling

