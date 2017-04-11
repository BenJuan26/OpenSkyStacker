
#-------------------------------------------------
#
# Project created by QtCreator 2017-03-14T21:16:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

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
        mainwindow.cpp \
    imagestacker.cpp \
    processingdialog.cpp \
    star.cpp \
    stardetector.cpp \
    pixel.cpp \
    adjoiningpixel.cpp \
    focas.cpp \
    stackergraphicsview.cpp \
    triangle.cpp

HEADERS  += mainwindow.h \
    imagestacker.h \
    processingdialog.h \
    star.h \
    stardetector.h \
    pixel.h \
    adjoiningpixel.h \
    focas.h \
    stackergraphicsview.h \
    hfti.h \
    triangle.h

FORMS    += mainwindow.ui \
    processingdialog.ui

win32 {
    INCLUDEPATH += $$(OPENCV_DIR)\build\include
    INCLUDEPATH += $$(LIBRAW_DIR)
    LIBS += -L$$(OPENCV_DIR)\build\lib\Release
    LIBS += -lopencv_core$$(OPENCV_VER)
    LIBS += -lopencv_highgui$$(OPENCV_VER)
    LIBS += -lopencv_imgcodecs$$(OPENCV_VER)
    LIBS += -lopencv_imgproc$$(OPENCV_VER)
    LIBS += -lopencv_features2d$$(OPENCV_VER)
    LIBS += -lopencv_calib3d$$(OPENCV_VER)
    LIBS += -lopencv_video$$(OPENCV_VER)
    LIBS += $$(FOCAS_DIR)\hfti.o
    LIBS += $$(FOCAS_DIR)\h12.o
    LIBS += $$(FOCAS_DIR)\diff.o
    LIBS += -L$$(LIBRAW_DIR)\buildfiles\Debug
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
    INCLUDEPATH += $$PWD/3rdparty/libraw/include
    LIBS += -L$$PWD/3rdparty/libraw/macx/lib \
        -lraw \
        -lraw_r

    # FOCAS
    LIBS += $$PWD/3rdparty/focas/macx/hfti.o
    LIBS += $$PWD/3rdparty/focas/macx/h12.o
    LIBS += $$PWD/3rdparty/focas/macx/diff.o
}
else:unix {
    INCLUDEPATH += $$(OPENCVDIR)/modules/core/include
    INCLUDEPATH += $$(OPENCVDIR)/release
    INCLUDEPATH += $$(OPENCVDIR)/modules/highgui/include
    INCLUDEPATH += $$(OPENCVDIR)/modules/imgproc/include
    INCLUDEPATH += $$(OPENCVDIR)/modules/video/include
    INCLUDEPATH += $$(OPENCVDIR)/modules/imgcodecs/include
    INCLUDEPATH += $$(OPENCVDIR)/modules/videoio/include
    INCLUDEPATH += $$(QTDIR)/include/QtWidgets
    LIBS += -L$$(OPENCVDIR)/release/lib \
        -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgcodecs \
        -lopencv_imgproc \
        -lopencv_features2d \
        -lopencv_calib3d \
        -lopencv_video \
        -lraw
    LIBS += /home/ben/Developer/focas/hfti.o
    LIBS += /home/ben/Developer/focas/h12.o
    LIBS += /home/ben/Developer/focas/diff.o
}
