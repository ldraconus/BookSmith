QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# For example, the following lines show how a library can be specified:
LIBS += -Ld:/Chris/OneDrive/Documents/src/libzip-win-build/build-VS2019/x64/Debug -llibzip-static
LIBS += -Ld:/Chris/OneDrive/Documents/src/zlib-win-build/build-VS2019/x64/Debug -llibz-static -lAdvapi32

# The paths containing header files can also be specified in a similar way using the INCLUDEPATH variable.
# For example, to add several paths to be searched for header files:
INCLUDEPATH = d:/Chris/OneDrive/Documents/src/libzip-win-build/lib d:/Chris/OneDrive/Documents/src/libzip-win-build/win32

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Zippy/Zippy.hpp \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
