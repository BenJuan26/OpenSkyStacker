
#-------------------------------------------------
#
# Project created by QtCreator 2017-03-14T21:16:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: QT += winextras

TARGET = OpenSkyStacker
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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
    model/imagerecord.cpp

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
    model/imagerecord.h

FORMS    += ui/mainwindow.ui \
    ui/processingdialog.ui

win32 {
    INCLUDEPATH += $$PWD/3rdparty/opencv/include
    INCLUDEPATH += $$PWD/3rdparty/libraw/win64/include
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
else:macx {
    # OpenCV
    INCLUDEPATH += $$PWD/3rdparty/opencv/include
    LIBS += -L$$PWD/3rdparty/opencv/macx/lib \
        -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgcodecs \
        -lopencv_imgproc \
        -lopencv_features2d \
        -lopencv_calib3d \
        -lopencv_video

    # LibRaw
    INCLUDEPATH += $$PWD/3rdparty/libraw/macx/include
    LIBS += -L$$PWD/3rdparty/libraw/macx/lib \
        -lraw \
        -lraw_r

    # FOCAS
    LIBS += $$PWD/3rdparty/focas/macx/hfti.o
    LIBS += $$PWD/3rdparty/focas/macx/h12.o
    LIBS += $$PWD/3rdparty/focas/macx/diff.o
}
else:unix {
    INCLUDEPATH += $$PWD/3rdparty/opencv/include
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
}
