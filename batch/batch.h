#ifndef OSS_BATCH_H
#define OSS_BATCH_H

#include <libstacker/imagestacker.h>
#include <QtCore>

using namespace openskystacker;

class OSSBatch : public QObject
{
    Q_OBJECT
    ImageStacker *stacker_;
    int progress_bar_width_;
    int max_message_length_ = 0;
    QString output_file_name_;

public:
    OSSBatch(QObject *parent = 0);

public slots:
    void PrintProgressBar(QString message, int percentage);
    void StackingFinished(cv::Mat, QString);
    void StackingError(QString);
    void Run();

signals:
    void Finished();
};


#endif // OSS_BATCH_H
