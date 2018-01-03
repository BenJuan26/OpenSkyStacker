#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include "libstacker/libstacker_global.h"
#include "libstacker/model.h"
#include "libstacker/stardetector.h"
#include "libstacker/util.h"

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QDebug>
#include <QTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QScopedPointer>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include <ctime>

#ifdef WIN32
#define LIBRAW_NODLL
#endif
#include <libraw.h>

#include <CCfits/CCfits>

namespace openskystacker {

// PIMPL class
class ImageStackerPrivate;
//! The main class for handling the image processing.
class LIBSTACKER_EXPORT ImageStacker : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ImageStacker)
    QScopedPointer<ImageStackerPrivate> const d_ptr;

#ifdef TEST_OSS
    friend class TestOSS;
#endif
public:
    //! Constructor for ImageStacker.
    explicit ImageStacker(QObject *parent = 0);
    ~ImageStacker();

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

} // namespace openskystacker

#endif // IMAGESTACKER_H
