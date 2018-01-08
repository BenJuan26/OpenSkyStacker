#include <batch.h>
#include <stdio.h>

OSSBatch::OSSBatch(QObject *parent) : QObject(parent),
    stacker_(new ImageStacker),
    progress_bar_width_(30)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    connect(stacker_, SIGNAL(UpdateProgress(QString,int)), this, SLOT(PrintProgressBar(QString,int)));
    connect(stacker_, SIGNAL(Finished(cv::Mat,QString)), this, SLOT(StackingFinished(cv::Mat,QString)));
}

void OSSBatch::PrintProgressBar(QString message, int percentage)
{
    QString p = QString(" %1% [").arg(QString::number(percentage).rightJustified(3, ' '));

    float progress = percentage / 100.0f;
    int pos = progress * progress_bar_width_;
    for (int i = 0; i < progress_bar_width_; i++) {
        if (i < pos) p += "=";
        else if (i == pos) p += ">";
        else p += " ";
    }

    p += QString("] %1").arg(message);
    if (p.size() > max_message_length_) {
        max_message_length_ = p.size();
    }
    printf("\r%s", p.leftJustified(max_message_length_, ' ').toUtf8().constData());
}

void OSSBatch::Run()
{

    QCommandLineParser parser;
    parser.setApplicationDescription("Multi-platform astroimaging stacker");
    parser.addHelpOption();

    QCommandLineOption listOption("f", tr("Image list JSON file."), "list");
    parser.addOption(listOption);

    QCommandLineOption outputOption("o", tr("Output image file."), "output");
    parser.addOption(outputOption);

    QCommandLineOption thresholdOption("t", tr("Star detection threshold (1-100). Default: 20"), "threshold", "20");
    parser.addOption(thresholdOption);

    QCommandLineOption threadsOption("j", tr("Number of processing threads. Default: 1"), "threads", "1");
    parser.addOption(threadsOption);

    parser.process(QCoreApplication::arguments());

    if (!parser.isSet(listOption)) {
        printf("Error: Must specify an image list");
        QCoreApplication::exit(1);
        return;
    }
    QString listFile = parser.value(listOption);

    if (!parser.isSet(outputOption)) {
        printf("Error: Must specify an output image");
        QCoreApplication::exit(1);
        return;
    }
    output_file_name_ = parser.value(outputOption);
    QRegularExpression regex("\\.tif$");
    if (!regex.match(output_file_name_).hasMatch()) {
        output_file_name_ += ".tif";
    }

    bool intParsingOk = true;
    int threshold = parser.value(thresholdOption).toInt(&intParsingOk);
    if (!intParsingOk) {
        printf("Error: Thread count argument must be an integer\n");
        QCoreApplication::exit(1);
        return;
    }

    intParsingOk = true;
    int threads = parser.value(threadsOption).toInt(&intParsingOk);
    if (!intParsingOk) {
        printf("Error: Thread count argument must be an integer\n");
        QCoreApplication::exit(1);
        return;
    }

    int err = 0;
    std::vector<ImageRecord *> records = LoadImageList(listFile, &err);
    if (err) {
        printf("Error %d: Couldn't load image list\n", err);
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
        stacker_->Process(threshold, threads);
    } catch (std::exception) {
        printf("Error: Got exception while stacking\n");
        QCoreApplication::exit(1);
        return;
    }

    //emit Finished();
}

void OSSBatch::StackingFinished(cv::Mat image, QString message)
{
    PrintProgressBar(message, 100);
    printf("\n");
    try {
        cv::imwrite(output_file_name_.toUtf8().constData(), image);
    } catch (std::exception) {
        printf("Error: Couldn't write resulting image\n");
        QCoreApplication::exit(1);
        return;
    }
    emit Finished();
}

void OSSBatch::StackingError(QString message)
{
    printf("Error: %s\n", message.toUtf8().constData());
    QCoreApplication::exit(1);
}
