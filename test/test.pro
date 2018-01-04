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
!exists( ../lib/libstacker* ) {
    error( "Compile libstacker before running tests!" )
}
LIBS += -L../lib -lstacker

macx {
    ICON = $$PWD/images/OpenSkyStacker.icns

    CONFIG += link_pkgconfig
    PKGCONFIG += libraw ccfits cfitsio

    # Not using pkg-config for OpenCV because we only want a few libs
    QMAKE_CXXFLAGS += $$system(pkg-config --cflags opencv)
    LIBS += $$system(pkg-config --libs-only-L opencv)
    LIBS += -lopencv_core
    LIBS += -lopencv_imgcodecs
    LIBS += -lopencv_imgproc

    # FOCAS
    LIBS += $$PWD/../libstacker/src/3rdparty/focas/macx/hfti.o
    LIBS += $$PWD/../libstacker/src/3rdparty/focas/macx/h12.o
    LIBS += $$PWD/../libstacker/src/3rdparty/focas/macx/diff.o
}

linux {
    CONFIG += link_pkgconfig
    PKGCONFIG += libraw cfitsio
    LIBS += -lCCfits

    # Not using pkg-config for OpenCV because we only want a few libs
    QMAKE_CXXFLAGS += $$system(pkg-config --cflags opencv)
    LIBS += $$system(pkg-config --libs-only-L opencv)
    LIBS += -lopencv_core
    LIBS += -lopencv_highgui
    opencv_libs = $$system(pkg-config --libs opencv)
    contains(opencv_libs,"imgcodecs"): LIBS += -lopencv_imgcodecs
    LIBS += -lopencv_imgproc

    LIBS += $$PWD/../libstacker/src/3rdparty/focas/unix/hfti.o
    LIBS += $$PWD/../libstacker/src/3rdparty/focas/unix/h12.o
    LIBS += $$PWD/../libstacker/src/3rdparty/focas/unix/diff.o

    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}
