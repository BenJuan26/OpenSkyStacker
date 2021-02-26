#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include "libstacker/libstacker_global.h"
#include "libstacker/model.h"
#include "libstacker/stardetector.h"
#include "libstacker/util.h"

#include <QObject>
#include <QScopedPointer>

#include <memory>


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
    ImageStacker(QObject *parent = 0);
    ~ImageStacker();

    //! Flag set to asynchronously cancel processing.
    bool cancel;

    // get/set
    QString getRefImageFileName() const;
    void setRefImageFileName(const QString &value);

    QStringList getTargetImageFileNames() const;
    void setTargetImageFileNames(const QStringList &value);

    QStringList getDarkFrameFileNames() const;
    void setDarkFrameFileNames(const QStringList &value);

    QStringList getDarkFlatFrameFileNames() const;
    void setDarkFlatFrameFileNames(const QStringList &value);

    QStringList getFlatFrameFileNames() const;
    void setFlatFrameFileNames(const QStringList &value);

    QString getSaveFilePath() const;
    void setSaveFilePath(const QString &value);

    cv::Mat getWorkingImage() const;
    void setWorkingImage(const cv::Mat &value);

    cv::Mat getRefImage() const;
    void setRefImage(const cv::Mat &value);

    cv::Mat getFinalImage() const;
    void setFinalImage(const cv::Mat &value);

    bool getUseDarks() const;
    void setUseDarks(bool value);

    bool getUseDarkFlats() const;
    void setUseDarkFlats(bool value);

    bool getUseFlats() const;
    void setUseFlats(bool value);

    QStringList getBiasFrameFileNames() const;
    void setBiasFrameFileNames(const QStringList &value);

    bool getUseBias() const;
    void setUseBias(bool value);

signals:
    //! Provides the final image and a message when processing is finished.
    /*!
        @param image The final processed image.
        @param message The message to accompany the process completion.
    */
    void finished(cv::Mat image, QString message);

    //! Provides a percentage of completion and a description of what's happening.
    /*!
        @param message A description of the current section of processing.
        @param percentComplete An integer, out of 100, representing the percentage of completion.
    */
    void updateProgress(QString message, int percentComplete);

    //! Emits a QImage after asynchronously reading it.
    void qImageReady(QImage image);

    //! Emitted when an error occurs during processing.
    void processingError(QString message);

    void doneDetectingStars(int);
public slots:
    //! The main method for processing the images.
    void process(int tolerance, int threads);

    //! Gets an image from disk and asynchronously reads it as a QImage.
    /*! Emits qImageReady once the image is ready.
        @param filename The image file to read.
    */
    void readQImage(QString filename);

    void detectStars(QString filename, int threshold);

}; // class ImageStacker

} // namespace openskystacker

#endif // IMAGESTACKER_H
