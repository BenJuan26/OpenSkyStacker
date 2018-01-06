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
