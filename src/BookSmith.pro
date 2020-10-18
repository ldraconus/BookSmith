#-------------------------------------------------
#
# Project created by QtCreator 2020-08-07T13:52:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BookSmith
TEMPLATE = app

CONFIG += c++17 embed_manifest_exe

SOURCES += main.cpp\
    finddialog.cpp \
    helpdialog.cpp \
        mainwindow.cpp \
    replacedialog.cpp \
    util.cpp \
    fullscreen.cpp \
    tagsdialog.cpp

HEADERS  += mainwindow.h \
    finddialog.h \
    helpdialog.h \
    replacedialog.h \
    util.h \
    fullscreen.h \
    tagsdialog.h

FORMS    += mainwindow.ui \
    finddialog.ui \
    fullscreen.ui \
    helpdialog.ui \
    replacedialog.ui \
    tagsdialog.ui

RESOURCES += \
    ../BookSmith.qrc
