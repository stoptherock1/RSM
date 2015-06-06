#-------------------------------------------------
#
# Project created by QtCreator 2015-06-06T19:59:16
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tstEnv
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    main_.cpp \
    testEnv.cpp

HEADERS  += MainWindow.h \
    testEnv.hpp

FORMS    += MainWindow.ui

DISTFILES += \
    main.dSYM \
    makefile
