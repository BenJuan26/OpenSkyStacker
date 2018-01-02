#ifndef UTIL_H
#define UTIL_H

#include "libstacker/libstacker_global.h"

#include "model/imagerecord.h"
#include "model/star.h"
#include "model/triangle.h"
#include "processing/stardetector.h"
#include "processing/exif.h"
#include "processing/focas.h"

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

struct LIBSTACKER_EXPORT StackingResult {
    cv::Mat image;
    int totalValidImages;
};

struct LIBSTACKER_EXPORT StackingParams {
    QStringList lights;
    cv::Mat ref;
    cv::Mat masterDark;
    cv::Mat masterFlat;
    cv::Mat masterBias;
    int tolerance;
    int threadIndex;
    int totalThreads;
};

LIBSTACKER_EXPORT cv::Mat GetBayerMatrix(QString filename);
LIBSTACKER_EXPORT ImageRecord *GetImageRecord(QString filename);
LIBSTACKER_EXPORT time_t EXIFTimeToCTime(std::string exifTime);
LIBSTACKER_EXPORT time_t FITSTimeToCTime(std::string fitsTime);
LIBSTACKER_EXPORT ImageType GetImageType(QString filename);
LIBSTACKER_EXPORT cv::Mat GetCalibratedImage(QString filename, cv::Mat dark, cv::Mat flat, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat ReadImage(QString filename);
LIBSTACKER_EXPORT cv::Mat FITSToMat(QString filename);
LIBSTACKER_EXPORT cv::Mat RawToMat(QString filename);
LIBSTACKER_EXPORT cv::Mat ConvertAndScaleImage(cv::Mat image);
LIBSTACKER_EXPORT std::vector<ImageRecord *> LoadImageList(QString filename, int *err);
LIBSTACKER_EXPORT QImage Mat2QImage(const cv::Mat &src);
LIBSTACKER_EXPORT cv::Mat GenerateAlignedImage(cv::Mat ref, cv::Mat target, int tolerance, int *err);
LIBSTACKER_EXPORT cv::Mat StackDarks(QStringList filenames, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat StackDarkFlats(QStringList filenames, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat StackFlats(QStringList filenames, cv::Mat darkFlat, cv::Mat bias);
LIBSTACKER_EXPORT cv::Mat StackBias(QStringList filenames);
LIBSTACKER_EXPORT StackingResult ProcessConcurrent(StackingParams params, int *numCompleted);
}

#endif // UTIL_H
