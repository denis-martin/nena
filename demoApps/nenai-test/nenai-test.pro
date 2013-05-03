# -------------------------------------------------
# Project created by QtCreator 2010-08-23T21:49:11
# -------------------------------------------------
TARGET = nenai-test
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    testthreads.cpp
HEADERS += mainwindow.h \
    testthreads.h
FORMS += mainwindow.ui

LIBS += -L../nenai -lnenai -lboost_system-mt -lboost_thread-mt

INCLUDEPATH += ../nenai
