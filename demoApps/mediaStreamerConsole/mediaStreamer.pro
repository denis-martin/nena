# @file mediaStreamer.pro
# @author Helge Backhaus

TEMPLATE = app
CONFIG -= debug
CONFIG += console release
QT += network
LIBS += -L/usr/lib64/ -lavformat -lavcodec -lavutil -lswscale
INCLUDEPATH += /usr/include/libavcodec /usr/include/libavformat /usr/include/libavutil /usr/include/libswscale
TARGET = mediaStreamer

# Input
HEADERS += definitions.h QMainThread.h QStreamer.h
SOURCES += main.cpp QMainThread.cpp QStreamer.cpp
