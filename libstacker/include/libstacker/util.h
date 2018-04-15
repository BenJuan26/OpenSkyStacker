#ifndef OSS_UTIL_H
#define OSS_UTIL_H

#include "libstacker/libstacker_global.h"
#include "libstacker/model.h"
#include "libstacker/stardetector.h"

#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTextStream>
#include <QImage>

#include <libraw.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <CCfits/CCfits>
#include <ctime>

namespace openskystacker {
const std::vector<QString> RAW_EXTENSIONS = {"3fr", "ari", "arw", "bay", "crw", "cr2",
        "cap", "data", "dcs", "dcr", "dng", "drf", "eip", "erf", "fff", "gpr", "iiq", "k25", "kdc",
        "mdc", "mef", "mos", "mrw", "nef", "nrw", "obm", "orf", "pef", "ptx", "pxn", "r3d", "raf",
        "raw", "rwl", "rw2", "rwz", "sr2", "srf", "srw", "x3f"};

const std::vector<QString> FITS_EXTENSIONS = {"fit", "fits", "fts"};

enum ImageType {
    RAW_IMAGE,
    FITS_IMAGE,
    RGB_IMAGE
};

struct StackingResult {
    cv::Mat image;
    int totalValidImages;
};

struct StackingParams {
    QStringList lights;
    cv::Mat ref;
    cv::Mat masterDark;
    cv::Mat masterFlat;
    cv::Mat masterBias;
    int tolerance;
    int threadIndex;
    int totalThreads;
};

LIBSTACKER_EXPORT cv::Mat getBayerMatrix(QString filename);
LIBSTACKER_EXPORT ImageRecord *getImageRecord(QString filename);
LIBSTACKER_EXPORT time_t exifTimeToCTime(std::string exifTime);
LIBSTACKER_EXPORT time_t fitsTimeToCTime(std::string fitsTime);
LIBSTACKER_EXPORT ImageType getImageType(QString filename);
LIBSTACKER_EXPORT cv::Mat getCalibratedImage(QString filename, cv::Mat dark, cv::Mat flat, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat readImage(QString filename);
LIBSTACKER_EXPORT cv::Mat fitsToMat(QString filename);
LIBSTACKER_EXPORT cv::Mat rawToMat(QString filename);
LIBSTACKER_EXPORT cv::Mat convertAndScaleImage(cv::Mat image);
LIBSTACKER_EXPORT std::vector<ImageRecord *> loadImageList(QString filename, int *err);
LIBSTACKER_EXPORT QImage mat2QImage(const cv::Mat &src);
LIBSTACKER_EXPORT cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target, int tolerance, int *err);
LIBSTACKER_EXPORT cv::Mat stackDarks(QStringList filenames, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat stackDarkFlats(QStringList filenames, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat stackFlats(QStringList filenames, cv::Mat darkFlat, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat stackBias(QStringList filenames);
LIBSTACKER_EXPORT StackingResult processConcurrent(StackingParams params, int *numCompleted);
}

#endif // OSS_UTIL_H
