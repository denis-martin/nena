# @file mediaPlayer.pro
# @author Helge Backhaus

TEMPLATE = app
CONFIG -= debug
CONFIG += release
QT += network
LIBS += -L/usr/lib64/ -lavformat -lavcodec -lavutil -lswscale -L../tmnet -ltmnet
INCLUDEPATH += /usr/include/libavcodec /usr/include/libavformat /usr/include/libavutil /usr/include/libswscale ..
TARGET = mediaPlayer

# Input
HEADERS += QMainView.h QViewer.h QNenaSocket.h
SOURCES += main.cpp QMainView.cpp QViewer.cpp QNenaSocket.cpp
