CONFIG += link_pkgconfig
PKGCONFIG += libraw cfitsio opencv
LIBS += -lCCfits

LIBS += $$PWD/src/3rdparty/focas/unix/hfti.o
LIBS += $$PWD/src/3rdparty/focas/unix/h12.o
LIBS += $$PWD/src/3rdparty/focas/unix/diff.o
