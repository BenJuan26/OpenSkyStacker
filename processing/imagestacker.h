#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include <QObject>
#include <opencv2/core.hpp>
#include <QMutex>
#include "stardetector.h"
#include "focas.h"
#include "model/imagerecord.h"
#include <QImage>

//! The main class for handling the image processing.
class ImageStacker : public QObject
{
    Q_OBJECT

#ifdef TEST_OSS
    friend class TestOSS;
#endif
public:
    explicit ImageStacker(QObject *parent = 0);
    bool cancel;

    //! The extensions that the app will treat as RAW images.
    static const std::vector<QString> RAW_EXTENSIONS;

    //! Used to define the number of bits of the final image.
    enum BITS_PER_CHANNEL{
        BITS_16, /*!< 16-bit image. */
        BITS_32  /*!< 32-bit image. */
    };

    //! Constructs an ImageRecord from the given file.
    /*!
        @param filename The image file to get the record from.
    */
    ImageRecord* getImageRecord(QString filename);

    cv::Mat readImage(QString filename);

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

    BITS_PER_CHANNEL getBitsPerChannel() const;
    void setBitsPerChannel(const BITS_PER_CHANNEL &value);

    QStringList getBiasFrameFileNames() const;
    void setBiasFrameFileNames(const QStringList &value);

    bool getUseBias() const;
    void setUseBias(bool value);

signals:
    //! Emitted when the processing is finished.
    /*!
        @param image The final processed image.
    */
    void finished(cv::Mat image);
    void finishedDialog(QString message);

    //! Provides a percentage of completion and a description of what's happening.
    void updateProgress(QString message, int percentComplete);
    void QImageReady(QImage image);

    //! Emitted when an error occurs during processing.
    void processingError(QString message);
public slots:
    //! The main method for processing the images.
    void process();
    void readQImage(QString filename);

private:
    //! Aligns the target image to the reference image and returns it.
    /*!
        @param ref The reference image.
        @param target The image to be aligned.
    */
    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);
    cv::Mat averageImages(cv::Mat img1, cv::Mat img2);

    //! Checks that all checked images are the same size
    /*!
        @return 0 if all images are validated; -1 otherwise.
    */
    int validateImageSizes();

    QImage Mat2QImage(const cv::Mat &src);

    BITS_PER_CHANNEL bitsPerChannel;

    void stackDarks();
    void stackDarkFlats();
    void stackFlats();
    void stackBias();

    mutable QMutex mutex;

    int currentOperation;
    int totalOperations;

    bool useDarks = false;
    bool useDarkFlats = false;
    bool useFlats = false;
    bool useBias = false;

    QString refImageFileName;
    QStringList targetImageFileNames;
    QStringList darkFrameFileNames;
    QStringList darkFlatFrameFileNames;
    QStringList flatFrameFileNames;
    QStringList biasFrameFileNames;

    QString saveFilePath;

    cv::Mat workingImage;
    cv::Mat refImage;
    cv::Mat finalImage;

    cv::Mat masterDark;
    cv::Mat masterDarkFlat;
    cv::Mat masterFlat;
    cv::Mat masterBias;

    cv::Mat convertAndScaleImage(cv::Mat image);
    cv::Mat rawToMat(QString filename);
    cv::Mat generateAlignedImageOld(cv::Mat ref, cv::Mat target);
};

#endif // IMAGESTACKER_H
