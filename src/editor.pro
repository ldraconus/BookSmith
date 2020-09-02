#-------------------------------------------------
#
# Project created by QtCreator 2020-08-07T13:52:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BookSmith
TEMPLATE = app

CONFIG += c++14

SOURCES += main.cpp\
        mainwindow.cpp \
    util.cpp \
    fullscreen.cpp \
    tagsdialog.cpp

HEADERS  += mainwindow.h \
    util.h \
    fullscreen.h \
    tagsdialog.h

FORMS    += mainwindow.ui \
    fullscreen.ui \
    tagsdialog.ui
