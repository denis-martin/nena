#
# Parameters:
#   debug=1 - compile in debug information
#   floatcount=1 - trace increments/decrements of out-floating-packets per flow state
#   msgtrace=1 - create a message trace of internal message files
#
# Targets:
#   <none> - compile Netlets, Multiplexers, and Daemon Core
#   boost - compile boost-target
#   omnet - compile omnet-target
#

import os
import subprocess

if ARGUMENTS.get('debug', 0):
	env = Environment(CCFLAGS = ['-g', '-Wall'])
else:
	env = Environment(CCFLAGS = ['-O2'])

cppdefines = []
if ARGUMENTS.get('floatcount', 0):
	cppdefines.append("FLOWSTATE_FLOATINGPACKETS_HISTORY")
if ARGUMENTS.get('msgtrace', 0):
	cppdefines.append("NENA_MESSAGE_TRACE")
env.Append(CPPDEFINES = cppdefines)

# needed by colorgcc
env.Append(ENV = {
	'PATH' : os.environ['PATH'],
	'TERM' : os.environ['TERM'],
	'HOME' : os.environ['HOME']
})

env.Append(CPPPATH=[
	'#/include', 
	'#/src/daemon',
	'#/src/archs', 
	'#/src/netlets',
	'#/include/archdep',
	'#/src/buildingBlocks',
	'/usr/local/include',
	'#/3rdparty',
	'#/src/archs/edu.kit.tm/itm/simpleArch'
])

env.Append(LIBS=[
	'reboost'
])

env.Append(LIBPATH=[
	'/usr/local/lib',
	'#/3rdparty/reboost',
])

env.Append(DEFINES=[])

Export('env')

env.SConscript('3rdparty/SConscript')
env.SConscript('demoApps/SConscript')
env.SConscript('src/SConscript', variant_dir='build', duplicate=0)


