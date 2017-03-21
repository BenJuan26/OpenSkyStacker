#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include <QObject>
#include <opencv2/core.hpp>

class ImageStacker : public QObject
{
    Q_OBJECT
public:
    explicit ImageStacker(QObject *parent = 0);
    bool cancel;

signals:
    void finished(cv::Mat image);
    void finishedDialog(QString message);
    void updateProgress(QString message, int percentComplete);
public slots:
    void process(QString refImageFileName, QStringList targetImageFileNames);

private:
    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);
    cv::Mat averageImages16UC3(cv::Mat img1, cv::Mat img2);

    int totalOperations;
};

#endif // IMAGESTACKER_H
