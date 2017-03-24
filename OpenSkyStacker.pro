
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
    processingdialog.cpp

HEADERS  += mainwindow.h \
    imagestacker.h \
    processingdialog.h

FORMS    += mainwindow.ui \
    processingdialog.ui

win32 {
    INCLUDEPATH += C:\OpenCV-Build\install\include
    INCLUDEPATH += C:\LibRaw-0.18.2
    LIBS += -LC:\OpenCV-Build\install\x86\mingw\lib \
        -lopencv_core310.dll \
        -lopencv_highgui310.dll \
        -lopencv_imgcodecs310.dll \
        -lopencv_imgproc310.dll \
        -lopencv_features2d310.dll \
        -lopencv_calib3d310.dll \
        -lopencv_video310.dll
    LIBS += C:\LibRaw-0.18.2\lib\libraw.a
    LIBS += -lWS2_32
}
macx {
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
}
unix {
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
        -lopencv_video
}
