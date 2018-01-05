QT += testlib
TARGET = testoss
SOURCES -= main.cpp
SOURCES += testoss.cpp
HEADERS += testoss.h
DEFINES += TEST_OSS

DESTDIR = ../bin
BUILD_DIR = ../build
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc
UI_DIR = $$BUILD_DIR/ui
OBJECTS_DIR = $$BUILD_DIR/o

# On macOS we don't want the app bundled; a standalone executable is fine.
CONFIG -= app_bundle

INCLUDEPATH += ../libstacker/include
LIBS += -L../lib -lstacker

win32: include(../libstacker/win32.pri)
macx: include(../libstacker/mac.pri)
linux: include(../libstacker/linux.pri)
