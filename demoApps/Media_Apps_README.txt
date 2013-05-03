The mediaApp is used for streaming webcam data and receiving video
streams (prerecorded or webcam). The mediaStreamer is used for streaming
a video file to the network. There is a GUI and a command line version.

Call ./mediaApp --help or ./mediaStreamer --help for list of command line options.

Parameters like framerate, resolution and video file can be adjusted in
the .conf files. The applications look for a .conf file in its local folder.
An alternate .conf file can be used via the --file "path" command line parameter of
each application.

For a basic setup use

./mediaApp --client

so that it will receive a video stream.

Then start the console version of the mediaStreamer without any command line parameters.
The console version connects to the nodearchitecture automatically, so make sure it is
already running prior to starting the application.

Also make sure you adjust the target, identifier and sender parameters in each applications
config file beforehand. There are a few different configurations to look at in the
examples folder.

To compile it, you will need the following packages:

  libavcodec-dev
  libavformat-dev
  libswscale-dev
  libcv-dev
  libcv4
  (libcvaux-dev and libhighgui-dev may also be required)

