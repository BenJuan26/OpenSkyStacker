#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include <QObject>
#include <opencv2/core.hpp>
#include <QMutex>

class ImageStacker : public QObject
{
    Q_OBJECT
public:
    explicit ImageStacker(QObject *parent = 0);
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

signals:
    void finished(cv::Mat image);
    void finishedDialog(QString message);
    void updateProgress(QString message, int percentComplete);
public slots:
    void process();

private:
    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);
    cv::Mat averageImages16UC3(cv::Mat img1, cv::Mat img2);

    mutable QMutex mutex;

    int totalOperations;

    QString refImageFileName;
    QStringList targetImageFileNames;
    QStringList darkFrameFileNames;
    QStringList darkFlatFrameFileNames;
    QStringList flatFrameFileNames;

    QString saveFilePath;

    cv::Mat workingImage;
    cv::Mat refImage;
    cv::Mat finalImage;

};

#endif // IMAGESTACKER_H
