
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
    triangle.cpp

HEADERS  += mainwindow.h \
    imagestacker.h \
    processingdialog.h \
    star.h \
    stardetector.h \
    pixel.h \
    adjoiningpixel.h \
    focas.h \
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
    INCLUDEPATH += /usr/local/Cellar/opencv3/3.2.0/include
    INCLUDEPATH += /usr/local/Cellar/libraw/0.17.2_1/include
    LIBS += -L/usr/local/lib \
        -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgcodecs \
        -lopencv_imgproc \
        -lopencv_features2d \
        -lopencv_calib3d \
        -lopencv_video \
        -lraw \
        -lraw_r
        -lgfortran
    LIBS += /Users/Ben/Downloads/focas/hfti.o
    LIBS += /Users/Ben/Downloads/focas/h12.o
    LIBS += /Users/Ben/Downloads/focas/diff.o
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
