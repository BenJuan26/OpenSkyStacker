#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include <QObject>
#include <opencv2/core.hpp>
#include <QMutex>
#include "stardetector.h"
#include "focas.h"

class ImageStacker : public QObject
{
    Q_OBJECT
public:
    explicit ImageStacker(QObject *parent = 0);
    bool cancel;

    static const std::vector<QString> RAW_EXTENSIONS;
    enum BITS_PER_CHANNEL{BITS_16, BITS_32};


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

signals:
    void finished(cv::Mat image);
    void finishedDialog(QString message);
    void updateProgress(QString message, int percentComplete);
public slots:
    void process();

private:
    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);
    cv::Mat averageImages(cv::Mat img1, cv::Mat img2);

    BITS_PER_CHANNEL bitsPerChannel;

    void stackDarks();
    void stackDarkFlats();
    void stackFlats();

    mutable QMutex mutex;

    int currentOperation;
    int totalOperations;

    bool useDarks = false;
    bool useDarkFlats = false;
    bool useFlats = false;

    QString refImageFileName;
    QStringList targetImageFileNames;
    QStringList darkFrameFileNames;
    QStringList darkFlatFrameFileNames;
    QStringList flatFrameFileNames;

    QString saveFilePath;

    cv::Mat workingImage;
    cv::Mat refImage;
    cv::Mat finalImage;

    cv::Mat masterDark;
    cv::Mat masterDarkFlat;
    cv::Mat masterFlat;

    cv::Mat convertAndScaleImage(cv::Mat image);
    cv::Mat rawToMat(QString filename);
    cv::Mat generateAlignedImageOld(cv::Mat ref, cv::Mat target);
};

#endif // IMAGESTACKER_H
