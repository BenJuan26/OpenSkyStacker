QT       += core

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/src

win32: QT += winextras

TARGET = stacker

TEMPLATE = lib

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS LIBSTACKER__

DESTDIR = ../lib
BUILD_DIR = ../build/libstacker
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc
UI_DIR = $$BUILD_DIR/ui
win32:OBJECTS_DIR = $$BUILD_DIR/o/win32
macx:OBJECTS_DIR = $$BUILD_DIR/o/macx
linux:OBJECTS_DIR = $$BUILD_DIR/o/linux

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
    

win32 {
    OPENCV_DIR = $$(OPENCV_DIR)
    OPENCV_VER = $$(OPENCV_VER)
    LIBRAW_DIR = $$(LIBRAW_DIR)
    CCFITS_INCLUDE = $$(CCFITS_INCLUDE)
    CCFITS_LIB = $$(CCFITS_LIB)
    CFITSIO_INCLUDE = $$(CFITSIO_INCLUDE)
    CFITSIO_LIB = $$(CFITSIO_LIB)

    isEmpty(OPENCV_DIR) {
        error(Must define the env var OPENCV_DIR that points to the OpenCV build directory.)
    }
    isEmpty(OPENCV_VER) {
        error(Must define the env var OPENCV_VER indicating the OpenCV version (ex. "320").)
    }
    isEmpty(LIBRAW_DIR) {
        error(Must define the env var LIBRAW_DIR that points to the LibRaw build directory.)
    }
    isEmpty(CCFITS_INCLUDE) {
        error(Must define the env var CCFITS_INCLUDE that points to the CCfits include files.)
    }
    isEmpty(CCFITS_LIB) {
        error(Must define the env var CCFITS_LIB that points to directory containing the CCfits library.)
    }
    isEmpty(CFITSIO_INCLUDE) {
        error(Must define the env var CFITSIO_INCLUDE that points to the cfitsio include files.)
    }
    isEmpty(CFITSIO_LIB) {
        error(Must define the env var CFITSIO_LIB that points to directory containing the cfitsio library.)
    }


    INCLUDEPATH += $$OPENCV_DIR/include
    INCLUDEPATH += $$LIBRAW_DIR/libraw
    INCLUDEPATH += $$CCFITS_INCLUDE
    INCLUDEPATH += $$CFITSIO_INCLUDE
    LIBS += -lucrt
    LIBS += -lucrtd
    LIBS += -L$$OPENCV_DIR/lib/Release
    LIBS += -lopencv_core$$OPENCV_VER
    LIBS += -lopencv_imgcodecs$$OPENCV_VER
    LIBS += -lopencv_imgproc$$OPENCV_VER
    LIBS += $$PWD/src/3rdparty/focas/win64/hfti.o
    LIBS += $$PWD/src/3rdparty/focas/win64/h12.o
    LIBS += $$PWD/src/3rdparty/focas/win64/diff.o
    LIBS += -L$$LIBRAW_DIR/lib
    LIBS += -llibraw
    LIBS += -lWS2_32
    LIBS += -L$$CCFITS_LIB -lCCfits
    LIBS += -L$$CFITSIO_LIB -lcfitsio

    # Don't use Microsoft's min and max functions
    DEFINES += NOMINMAX
}

macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libraw ccfits cfitsio

    # Not using pkg-config for OpenCV because we only want a few libs
    QMAKE_CXXFLAGS += $$system(pkg-config --cflags opencv)
    LIBS += $$system(pkg-config --libs-only-L opencv)
    LIBS += -lopencv_core
    LIBS += -lopencv_imgcodecs
    LIBS += -lopencv_imgproc

    # FOCAS
    LIBS += $$PWD/src/3rdparty/focas/macx/hfti.o
    LIBS += $$PWD/src/3rdparty/focas/macx/h12.o
    LIBS += $$PWD/src/3rdparty/focas/macx/diff.o
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

    LIBS += $$PWD/src/3rdparty/focas/unix/hfti.o
    LIBS += $$PWD/src/3rdparty/focas/unix/h12.o
    LIBS += $$PWD/src/3rdparty/focas/unix/diff.o
}
