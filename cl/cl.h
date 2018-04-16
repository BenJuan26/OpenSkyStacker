#ifndef OSS_CL_H
#define OSS_CL_H

#include <libstacker/imagestacker.h>
#include <QtCore>

using namespace openskystacker;

class OSS : public QObject
{
    Q_OBJECT
    ImageStacker *stacker;
    QThread *workerThread;
    int progressBarWidth;
    int maxMessageLength = 0;
    QString outputFileName;

public:
    OSS(QObject *parent = 0);
    ~OSS();

public slots:
    void printProgressBar(QString message, int percentage);
    void stackingFinished(cv::Mat, QString);
    void starDetectionFinished(int stars);
    void stackingError(QString);
    void run();

signals:
    void stackImages(int, int);
    void detectStars(QString, int);
    void finished();
};


#endif // OSS_CL_H
