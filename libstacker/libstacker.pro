QT       += core

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/src

win32: QT += winextras

TARGET = stacker

TEMPLATE = lib

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS LIBSTACKER__

DESTDIR = ../bin
BUILD_DIR = ../build/libstacker
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc
UI_DIR = $$BUILD_DIR/ui
OBJECTS_DIR = $$BUILD_DIR/o

QMAKE_DISTCLEAN += -r $$BUILD_DIR

SOURCES += src/adjoiningpixel.cpp \
    src/exif.cpp \
    src/focas.cpp \
    src/imagestacker.cpp \
    src/stardetector.cpp \
    src/util.cpp

HEADERS += include/libstacker/imagestacker.h \
    include/libstacker/model.h \
    include/libstacker/stardetector.h \
    include/libstacker/util.h \
    src/adjoiningpixel.h \
    src/exif.h \
    src/focas.h \
    src/hfti.h

isEmpty(PREFIX): PREFIX=/usr/local
isEmpty(LIBDIR): LIBDIR=$${PREFIX}/lib

includes.files = include/libstacker/*
includes.path = $${PREFIX}/include/libstacker
target.path = $${LIBDIR}
unix:INSTALLS += target includes

unix:coverage {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

win32: include(win32.pri)
macx: include(mac.pri)
linux: include(linux.pri)
