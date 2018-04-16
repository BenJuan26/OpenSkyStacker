#ifndef IMAGESTACKER_P_H
#define IMAGESTACKER_P_H

#include "libstacker/imagestacker.h"

#include <QMutex>

namespace openskystacker {

class ImageStackerPrivate
{
    Q_DISABLE_COPY(ImageStackerPrivate)
    Q_DECLARE_PUBLIC(ImageStacker)
    ImageStacker * const q_ptr;

public:
    ImageStackerPrivate(ImageStacker *parent = 0);

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

//public slots:
    //! The main method for processing the images.
    void process(int tolerance, int threads);

    //! Gets an image from disk and asynchronously reads it as a QImage.
    /*! Emits QImageReady once the image is ready.
        @param filename The image file to read.
    */
    void readQImage(QString filename);

    void detectStars(QString filename, int threshold);

private:
    int getTotalOperations();

    int validateImageSizes();

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

}; // class ImageStackerPrivate

} // namespace openskystacker

#endif
