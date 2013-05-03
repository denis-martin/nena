# @file mediaApp.pro
# @author Helge Backhaus

TEMPLATE = app
CONFIG -= debug
CONFIG += release link_pkgconfig
PKGCONFIG += opencv
QT += network
TARGET = mediaApp

# Input
HEADERS += definitions.h QMainView.h QWebCam.h QViewer.h
SOURCES += main.cpp QMainView.cpp QWebCam.cpp QViewer.cpp
