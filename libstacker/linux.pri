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
