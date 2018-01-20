QT       += core gui

TRANSLATIONS += translations/openskystacker_es.ts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32 {
    QT += winextras
    RC_ICONS = $$PWD/images/OpenSkyStacker.ico
}

macx: ICON = $$PWD/images/OpenSkyStacker.icns

TARGET = OpenSkyStacker
linux:TARGET = openskystacker

TEMPLATE = app

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR = ../bin
BUILD_DIR = ../build/openskystacker
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc
UI_DIR = $$BUILD_DIR/ui
OBJECTS_DIR = $$BUILD_DIR/o/win32

QMAKE_DISTCLEAN += -r $$BUILD_DIR

SOURCES += main.cpp\
    ui/mainwindow.cpp \
    ui/processingdialog.cpp \
    ui/stackergraphicsview.cpp \
    ui/imagetablemodel.cpp \
    ui/optionsdialog.cpp

HEADERS  += ui/mainwindow.h \
    ui/processingdialog.h \
    ui/stackergraphicsview.h \
    ui/imagetablemodel.h \
    ui/optionsdialog.h

FORMS    += ui/mainwindow.ui \
    ui/processingdialog.ui \
    ui/optionsdialog.ui

INCLUDEPATH += ../libstacker/include
LIBS += -L../lib -lstacker

isEmpty(PREFIX): PREFIX = /usr/local

target.path = $${PREFIX}/bin
unix: INSTALLS += target

desktop.files = ../openskystacker.desktop
desktop.path = $${PREFIX}/share/applications
icon.files = images/OpenSkyStacker.ico
icon.path = $${PREFIX}/share/pixmaps
linux: INSTALLS += desktop icon

win32: include(../libstacker/win32.pri)
macx: include(../libstacker/mac.pri)
linux: include(../libstacker/linux.pri)
