#ifndef IMAGESTACKER_H
#define IMAGESTACKER_H

#include <QObject>
#include <opencv2/core.hpp>

class ImageStacker : public QObject
{
    Q_OBJECT
public:
    explicit ImageStacker(QObject *parent = 0);

signals:
    void finished(cv::Mat image);
    void updateProgressBar(int value);
public slots:
    void process(QString refImageFileName, QStringList targetImageFileNames);

private:
    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);
    cv::Mat averageImages(cv::Mat img1, cv::Mat img2);
};

#endif // IMAGESTACKER_H
