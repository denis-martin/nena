# @file stateViewer.pro
# @author Helge Backhaus

TEMPLATE = app
CONFIG -= debug
CONFIG += release
QT += network script
TARGET = stateViewer

INCLUDEPATH += ..
LIBS += -L../tmnet -ltmnet

# Input
HEADERS += definitions.h QMainView.h QNenaConnector.h QNetletItem.h QBoxItem.h QMultiplexerItem.h QConnectorItem.h QArrowItem.h
SOURCES += main.cpp QMainView.cpp QNenaConnector.cpp QNetletItem.cpp QBoxItem.cpp QMultiplexerItem.cpp QConnectorItem.cpp QArrowItem.cpp
