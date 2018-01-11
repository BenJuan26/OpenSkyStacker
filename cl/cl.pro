CONFIG += c++11 console
CONFIG -= app_bundle
DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
    cl.h

SOURCES += main.cpp \
    cl.cpp

DESTDIR = ../bin
BUILD_DIR = ../build/cl
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc
UI_DIR = $$BUILD_DIR/ui
OBJECTS_DIR = $$BUILD_DIR/o

TARGET = oss_cl

# On macOS we don't want the app bundled; a standalone executable is fine.
CONFIG -= app_bundle

INCLUDEPATH += ../libstacker/include
LIBS += -L../lib -lstacker

win32: include(../libstacker/win32.pri)
macx: include(../libstacker/mac.pri)
linux: include(../libstacker/linux.pri)

