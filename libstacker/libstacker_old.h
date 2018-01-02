#ifndef LIBSTACKER_H
#define LIBSTACKER_H

#include "libstacker/libstacker_global.h"

#include <QString>
#include <QStringList>
#include <QObject>
#include <QColor>
#include <QTypeInfo>
#include <QImage>
#include <QRect>
#include <QPaintDevice>


#include <opencv2/core/core.hpp>

namespace openskystacker {

//! The main class for handling the image processing.
class LIBSTACKER_EXPORT ImageStacker : public QObject
{
    Q_OBJECT

#ifdef TEST_OSS
    friend class TestOSS;
#endif
public:
    //! Constructor for ImageStacker.
    explicit ImageStacker(QObject *parent = 0);

    //! Flag set to asynchronously cancel processing.
    bool cancel_;

    // get/set
    QString GetRefImageFileName() const;
    void SetRefImageFileName(const QString &value);

    QStringList GetTargetImageFileNames() const;
    void SetTargetImageFileNames(const QStringList &value);

    QStringList GetDarkFrameFileNames() const;
    void SetDarkFrameFileNames(const QStringList &value);

    QStringList GetDarkFlatFrameFileNames() const;
    void SetDarkFlatFrameFileNames(const QStringList &value);

    QStringList GetFlatFrameFileNames() const;
    void SetFlatFrameFileNames(const QStringList &value);

    QString GetSaveFilePath() const;
    void SetSaveFilePath(const QString &value);

    cv::Mat GetWorkingImage() const;
    void SetWorkingImage(const cv::Mat &value);

    cv::Mat GetRefImage() const;
    void SetRefImage(const cv::Mat &value);

    cv::Mat GetFinalImage() const;
    void SetFinalImage(const cv::Mat &value);

    bool GetUseDarks() const;
    void SetUseDarks(bool value);

    bool GetUseDarkFlats() const;
    void SetUseDarkFlats(bool value);

    bool GetUseFlats() const;
    void SetUseFlats(bool value);

    QStringList GetBiasFrameFileNames() const;
    void SetBiasFrameFileNames(const QStringList &value);

    bool GetUseBias() const;
    void SetUseBias(bool value);

signals:
    //! Provides the final image and a message when processing is finished.
    /*!
        @param image The final processed image.
        @param message The message to accompany the process completion.
    */
    void Finished(cv::Mat image, QString message);

    //! Provides a percentage of completion and a description of what's happening.
    /*!
        @param message A description of the current section of processing.
        @param percentComplete An integer, out of 100, representing the percentage of completion.
    */
    void UpdateProgress(QString message, int percentComplete);

    //! Emits a QImage after asynchronously reading it.
    void QImageReady(QImage image);

    //! Emitted when an error occurs during processing.
    void ProcessingError(QString message);

    void doneDetectingStars(int);
public slots:
    //! The main method for processing the images.
    void Process(int tolerance, int threads);

    //! Gets an image from disk and asynchronously reads it as a QImage.
    /*! Emits QImageReady once the image is ready.
        @param filename The image file to read.
    */
    void ReadQImage(QString filename);

    void detectStars(QString filename, int threshold);

}; // class ImageStacker

//! Represents a star as it appears in an image.
class LIBSTACKER_EXPORT Star
{
public:
    //! Constructor.
    Star();

    //! Constructor.
    /*! @param x_ The x coordinate of the Star.
        @param y_ The y coordinate of the Star.
        @param value_ The intensity of the Star.
    */
    Star(int x, int y, float value);

    //! Destructor.
    ~Star();

    bool operator==(const Star& s);
    bool operator>(const Star& other) const;
    bool operator<(const Star& other) const;

    int GetX() const;
    void SetX(int value_);

    int GetY() const;
    void SetY(int value_);

    float GetPeak() const;
    void SetPeak(float value_);

    float GetRadius() const;
    void SetRadius(float value_);

    int GetArea() const;
    void SetArea(int value_);

    float GetValue() const;
    void SetValue(float value);

}; // class Star

//! Manages star analysis and detection.
class LIBSTACKER_EXPORT StarDetector
{
public:
    //! Constructor (currently empty).
    StarDetector();

    //! Destructor (currently empty).
    ~StarDetector();

    //! Analyzes the given image and returns a list of detected stars.
    /*!
        @param image The image containing the stars.
        @return A list of detected stars.
    */
    std::vector<Star> GetStars(cv::Mat image, int thresholdCoeff);
    std::vector<Star> GetStars(cv::Mat image);

    //! Gets the value of the pixel at the given coordinates, truncating to the edges if the coordinates are outside the image bounds.
    /*!
        @param image The image to get the pixel value from.
        @param x The x coordinate of the desired pixel.
        @param y The y coordinate of the desired pixel.
        @return The value of the pixel at (x,y), truncated to the bounds of the image.
    */
    float GetExtendedPixelValue(cv::Mat image, int x, int y);

    //! Generates approximately what the sky background would look like without the stars.
    /*!
        @param image The full image with stars.
        @return The sky background.
    */
    cv::Mat GenerateSkyBackground(cv::Mat image);

    using size_type = std::vector<Star>::size_type;
    //! Draw a white-on-black plot of the provided stars.
    /*! This started as a debugging tool but ended up being really cool and potentially useful.
        @param path Filename for the output image.
        @param width The width of the resulting image in pixels.
        @param height The height of the resulting image in pixels.
        @param limit Draw only the brightest N stars, or all of the stars if a negative value is given.
        @param stars The list of stars to draw.
    */
    void DrawDetectedStars(const std::string& path, uint width, uint height, size_type limit, std::vector<Star> stars);
}; // class StarDetector

#include <ctime>

//! Contains metadata regarding an image.
class LIBSTACKER_EXPORT ImageRecord
{
public:
    //! Constructor.
    ImageRecord();

    //! Describes the type of frame (e.g. Light, Dark, etc.).
    enum FrameType {
        LIGHT,     /*!< Light frame. */
        DARK,      /*!< Dark frame. */
        DARK_FLAT, /*!< Dark flat frame. */
        FLAT,      /*!< Flat frame. */
        BIAS       /*!< Bias/offset frame. */
    };

    QString GetFilename() const;
    void SetFilename(const QString &value);

    FrameType GetType() const;
    void SetType(const FrameType &value);

    float GetShutter() const;
    void SetShutter(float value);

    float GetIso() const;
    void SetIso(float value);

    bool IsReference() const;
    void SetReference(bool value);

    time_t GetTimestamp() const;
    void SetTimestamp(const time_t &value);

    bool IsChecked() const;
    void SetChecked(bool value);

    int GetWidth() const;
    void SetWidth(int value);

    int GetHeight() const;
    void SetHeight(int value);
}; // class ImageRecord

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

} // namespace openskystacker

#endif // LIBSTACKER_H