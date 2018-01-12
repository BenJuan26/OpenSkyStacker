#ifndef OSS_CL_H
#define OSS_CL_H

#include <libstacker/imagestacker.h>
#include <QtCore>

using namespace openskystacker;

class OSS : public QObject
{
    Q_OBJECT
    ImageStacker *stacker_;
    QThread *worker_thread_;
    int progress_bar_width_;
    int max_message_length_ = 0;
    QString output_file_name_;

public:
    OSS(QObject *parent = 0);
    ~OSS();

public slots:
    void PrintProgressBar(QString message, int percentage);
    void StackingFinished(cv::Mat, QString);
    void StackingError(QString);
    void Run();

signals:
    void StackImages(int, int);
    void Finished();
};


#endif // OSS_CL_H
