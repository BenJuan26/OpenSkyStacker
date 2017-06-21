
#-------------------------------------------------
#
# Project created by QtCreator 2017-03-14T21:16:33
#
#-------------------------------------------------

QT       += core gui

TRANSLATIONS += openskystacker_es.ts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: QT += winextras

TARGET = OpenSkyStacker
SOURCES += main.cpp

test {
    QT += testlib
    TARGET = testoss
    SOURCES -= main.cpp
    SOURCES += test/testoss.cpp
    DEFINES += TEST_OSS

    # On macOS we don't want the app bundled; a standalone executable is fine.
    CONFIG -= app_bundle
}

TEMPLATE = app

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


DESTDIR = $$PWD/build

OBJECTS_DIR = $$DESTDIR/obj
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/qrc
UI_DIR = $$DESTDIR/ui

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#SOURCES += main.cpp\
SOURCES += \
        ui/mainwindow.cpp \
    processing/imagestacker.cpp \
    ui/processingdialog.cpp \
    model/star.cpp \
    processing/stardetector.cpp \
    model/pixel.cpp \
    model/adjoiningpixel.cpp \
    processing/focas.cpp \
    ui/stackergraphicsview.cpp \
    model/triangle.cpp \
    model/imagerecord.cpp \
    model/imagetablemodel.cpp

HEADERS  += ui/mainwindow.h \
    processing/imagestacker.h \
    ui/processingdialog.h \
    model/star.h \
    processing/stardetector.h \
    model/pixel.h \
    model/adjoiningpixel.h \
    processing/focas.h \
    ui/stackergraphicsview.h \
    processing/hfti.h \
    model/triangle.h \
    model/imagerecord.h \
    model/imagetablemodel.h

FORMS    += ui/mainwindow.ui \
    ui/processingdialog.ui

win32 {
    RC_ICONS = $$PWD/OpenSkyStacker.ico

    INCLUDEPATH += $$PWD/3rdparty/opencv/include
    INCLUDEPATH += $$PWD/3rdparty/libraw/win64/include
    LIBS += -lucrt
    LIBS += -lucrtd
    LIBS += -L$$PWD/3rdparty/opencv/win64/lib
    LIBS += -lopencv_core320
    LIBS += -lopencv_highgui320
    LIBS += -lopencv_imgcodecs320
    LIBS += -lopencv_imgproc320
    LIBS += -lopencv_features2d320
    LIBS += -lopencv_calib3d320
    LIBS += -lopencv_video320
    LIBS += $$PWD/3rdparty/focas/win64/hfti.o
    LIBS += $$PWD/3rdparty/focas/win64/h12.o
    LIBS += $$PWD/3rdparty/focas/win64/diff.o
    LIBS += -L$$PWD/3rdparty/libraw/win64/lib
    LIBS += -lraw
    LIBS += -lWS2_32
}

macx {
    ICON = $$PWD/OpenSkyStacker.icns

    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib

    # OpenCV
    # INCLUDEPATH += $$PWD/3rdparty/opencv/include
    # INCLUDEPATH += $$PWD/3rdparty/opencv/build
    LIBS += \
        -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgcodecs \
        -lopencv_imgproc \
        -lopencv_features2d \
        -lopencv_calib3d \
        -lopencv_video

    opencv.target = /usr/local/lib/libopencv_core.dylib
    opencv.commands = source ~/.bash_profile;\
        cd $$PWD/3rdparty/opencv/build;\
        /Applications/CMake.app/Contents/bin/cmake -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=OFF \
            -DCMAKE_OSX_ARCHITECTURES=x86_64 -DWITH_1394=OFF -DWITH_FFMPEG=OFF ..;\
        make;\
        make install

    QMAKE_EXTRA_TARGETS += opencv
    PRE_TARGETDEPS += /usr/local/lib/libopencv_core.dylib


    # LibRaw
    # INCLUDEPATH += $$PWD/3rdparty/libraw
    LIBS += \
        -lraw \
        -lraw_r

    libraw.target = /usr/local/lib/libraw.dylib
    # sourcing .bash_profile is for includes and paths
    libraw.commands = source ~/.bash_profile; cd $$PWD/3rdparty/libraw; ./configure; make; make install

    QMAKE_EXTRA_TARGETS += libraw
    PRE_TARGETDEPS += /usr/local/lib/libraw.dylib

    # FOCAS
    LIBS += $$PWD/3rdparty/focas/macx/hfti.o
    LIBS += $$PWD/3rdparty/focas/macx/h12.o
    LIBS += $$PWD/3rdparty/focas/macx/diff.o

}

linux {
    INCLUDEPATH += $$PWD/3rdparty/opencv/include
    INCLUDEPATH += $$PWD/3rdparty/libraw/unix/include
    INCLUDEPATH += $$(QTDIR)/include/QtWidgets
    LIBS += -L$$PWD/3rdparty/opencv/unix/lib \
        -L$$PWD/3rdparty/libraw/unix/lib \
        -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgcodecs \
        -lopencv_imgproc \
        -lopencv_features2d \
        -lopencv_calib3d \
        -lopencv_video \
        -lraw
    LIBS += $$PWD/3rdparty/focas/unix/hfti.o
    LIBS += $$PWD/3rdparty/focas/unix/h12.o
    LIBS += $$PWD/3rdparty/focas/unix/diff.o

    snapcraft: {
        opencv.path = /usr/lib
        opencv.files = $$PWD/3rdparty/opencv/unix/lib/*

        libraw.path = /usr/lib
        libraw.files = $$PWD/3rdparty/libraw/unix/lib/*

        target.path = /bin
        INSTALLS += target opencv libraw
    }
}
