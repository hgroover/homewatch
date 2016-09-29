#-------------------------------------------------
#
# Project created by QtCreator 2016-09-17T02:57:42
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4) {
    QT       += serialport
} else {
    include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += multimedia
QT += network

TARGET = pimonitor
TEMPLATE = app

# Linux-only - this will build on Ubuntu or on raspbian
#INCLUDEPATH += `pkg-config --cflags libpulse-simple`
#LIBS += `pkg-config --libs libpulse-simple`
LIBS += -lpulse-simple

SOURCES += main.cpp\
        mainwindow.cpp \
    fault.cpp \
    pulsesimple.cpp \
    generator.cpp

HEADERS  += mainwindow.h \
    fault.h \
    pulsesimple.h \
    generator.h

FORMS    += mainwindow.ui
