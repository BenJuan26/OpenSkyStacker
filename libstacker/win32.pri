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
#LIBS += $$PWD/src/3rdparty/focas/win64/hfti.o
LIBS += $$PWD/src/3rdparty/focas/win64/h12.o
LIBS += $$PWD/src/3rdparty/focas/win64/diff.o
LIBS += -L$$LIBRAW_DIR/lib
LIBS += -llibraw
LIBS += -lWS2_32
LIBS += -L$$CCFITS_LIB -lCCfits
LIBS += -L$$CFITSIO_LIB -lcfitsio

# Don't use Microsoft's min and max functions
DEFINES += NOMINMAX
