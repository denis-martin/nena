# @file mediaStreamer.pro
# @author Helge Backhaus

TEMPLATE = app
CONFIG -= debug
CONFIG += release
QT += network
TARGET = mediaStreamer
LIBS += -L/usr/lib64/ -lavformat -lavcodec -lavutil -lswscale
INCLUDEPATH += /usr/include/libavcodec /usr/include/libavformat /usr/include/libavutil /usr/include/libswscale

# Input
HEADERS += definitions.h QMainView.h QStreamer.h
SOURCES += main.cpp QMainView.cpp QStreamer.cpp
