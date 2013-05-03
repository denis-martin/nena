# @file mediaServer.pro
# @author Helge Backhaus

TEMPLATE = app
CONFIG -= debug
CONFIG += console release
QT += network script
LIBS += -L/usr/lib64/ -lavformat -lavcodec -lavutil -L../tmnet -ltmnet
INCLUDEPATH += /usr/include/libavcodec /usr/include/libavformat /usr/include/libavutil ..
TARGET = mediaServer

# Input
HEADERS += QMainThread.h QNenaServer.h
SOURCES += main.cpp QMainThread.cpp QNenaServer.cpp
