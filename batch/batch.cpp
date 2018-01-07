#include <batch.h>
#include <stdio.h>

OSSBatch::OSSBatch(int argc, char *argv[], QObject *parent) : QObject(parent),
    stacker_(new ImageStacker),
    argc_(argc),
    argv_(argv),
    progress_bar_width_(30)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    connect(stacker_, SIGNAL(UpdateProgress(QString,int)), this, SLOT(PrintProgressBar(QString,int)));
    connect(stacker_, SIGNAL(Finished(cv::Mat,QString)), this, SLOT(StackingFinished(cv::Mat,QString)));
}

void OSSBatch::PrintProgressBar(QString message, int percentage)
{
    std::printf("\r %3d%% [", percentage);
    float progress = percentage / 100.0f;

    int pos = progress * progress_bar_width_;
    for (int i = 0; i < progress_bar_width_; i++) {
        if (i < pos) std::printf("=");
        else if (i == pos) std::printf(">");
        else std::printf(" ");
    }
    std::printf("] %s", message.toUtf8().constData());
}

void OSSBatch::Run()
{
//    PrintProgressBar("Testing", 50);
//    QThread::sleep(2);
//    PrintProgressBar("Testing again", 75);

    int err = 0;
    std::vector<ImageRecord *> records = LoadImageList("F:/Astro/Samples/M42/all.json", &err);
    if (err) {
        std::printf("Error: Couldn't load image list");
        QCoreApplication::exit(1);
        return;
    }

    QString ref;
    QStringList lights;
    QStringList darks;
    QStringList darkFlats;
    QStringList flats;
    QStringList bias;
    bool referenceSet = false;
    for (ImageRecord *record : records) {
        if (!record->checked)
            continue;

        QString filename = record->filename;

        switch(record->type) {
        case ImageRecord::LIGHT:
            if (!referenceSet) {
                ref = filename;
                referenceSet = true;
            } else {
                lights.append(filename);
            }
            break;
        case ImageRecord::DARK:
            darks.append(filename);
            stacker_->SetUseDarks(true);
            break;
        case ImageRecord::DARK_FLAT:
            darkFlats.append(filename);
            stacker_->SetUseDarkFlats(true);
            break;
        case ImageRecord::FLAT:
            flats.append(filename);
            stacker_->SetUseFlats(true);
            break;
        case ImageRecord::BIAS:
            bias.append(filename);
            stacker_->SetUseBias(true);
            break;
        }
    }

    stacker_->SetRefImageFileName(ref);
    stacker_->SetTargetImageFileNames(lights);
    stacker_->SetDarkFrameFileNames(darks);
    stacker_->SetDarkFlatFrameFileNames(darkFlats);
    stacker_->SetFlatFrameFileNames(flats);
    stacker_->SetBiasFrameFileNames(bias);

    try {
        //stacker_->Process(threshold, 1);
        stacker_->Process(18, 1);
    } catch (std::exception) {
        std::printf("Error: Got exception while stacking");
        QCoreApplication::exit(1);
        return;
    }

    //emit Finished();
}

void OSSBatch::StackingFinished(cv::Mat image, QString message)
{
    PrintProgressBar(message, 100);
    std::printf("\n");
    try {
        cv::imwrite("F:/Astro/Samples/M42/consoletest.tif", image);
    } catch (std::exception) {
        std::printf("Error: Couldn't write resulting image");
        QCoreApplication::exit(1);
        return;
    }
    emit Finished();
}
