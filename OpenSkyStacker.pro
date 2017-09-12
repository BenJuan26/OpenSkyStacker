
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

TEMPLATE = app

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = $$PWD/build

OBJECTS_DIR = $$DESTDIR/obj
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/qrc
UI_DIR = $$DESTDIR/ui

SOURCES += main.cpp\
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
    model/imagetablemodel.cpp \
    ui/optionsdialog.cpp

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
    model/imagetablemodel.h \
    ui/optionsdialog.h

FORMS    += ui/mainwindow.ui \
    ui/processingdialog.ui \
    ui/optionsdialog.ui

test {
    QT += testlib
    TARGET = testoss
    SOURCES -= main.cpp
    SOURCES += test/testoss.cpp
    HEADERS += test/testoss.h
    DEFINES += TEST_OSS

    # On macOS we don't want the app bundled; a standalone executable is fine.
    CONFIG -= app_bundle
}

win32 {
    RC_ICONS = $$PWD/OpenSkyStacker.ico

    INCLUDEPATH += $$PWD/3rdparty/opencv/build/include
    INCLUDEPATH += $$PWD/3rdparty/libraw/libraw
    LIBS += -lucrt
    LIBS += -lucrtd
    LIBS += -L$$PWD/3rdparty/opencv/build/lib/Release
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
    LIBS += -L$$PWD/3rdparty/libraw/lib
    LIBS += -llibraw
    LIBS += -lWS2_32
}

macx {
    ICON = $$PWD/OpenSkyStacker.icns

    # OpenCV
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv libraw

    # FOCAS
    LIBS += $$PWD/3rdparty/focas/macx/hfti.o
    LIBS += $$PWD/3rdparty/focas/macx/h12.o
    LIBS += $$PWD/3rdparty/focas/macx/diff.o

}

linux {

    INCLUDE += /usr/include /usr/local/include /usr/include/x86_64-linux-gnu

    CONFIG += link_pkgconfig
    PKGCONFIG += opencv libraw

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
