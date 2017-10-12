#ifndef UTIL_H
#define UTIL_H

#include "model/imagerecord.h"
#include "processing/exif.h"

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
#include <opencv2/imgcodecs/imgcodecs.hpp>
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

cv::Mat GetBayerMatrix(QString filename);
ImageRecord *GetImageRecord(QString filename);
time_t EXIFTimeToCTime(std::string exifTime);
time_t FITSTimeToCTime(std::string fitsTime);
ImageType GetImageType(QString filename);
cv::Mat GetCalibratedImage(QString filename, cv::Mat dark, cv::Mat flat, cv::Mat bias);
cv::Mat ReadImage(QString filename);
cv::Mat FITSToMat(QString filename);
cv::Mat RawToMat(QString filename);
cv::Mat ConvertAndScaleImage(cv::Mat image);
std::vector<ImageRecord *> LoadImageList(QString filename, int *err);
QImage Mat2QImage(const cv::Mat &src);
}

#endif // UTIL_H
