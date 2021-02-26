#include <cl.h>

#include <stdio.h>
#include <iostream>

using namespace std;

OSS::OSS(QObject *parent) : QObject(parent),
    stacker(new ImageStacker),
    workerThread(new QThread),
    progressBarWidth(30)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    stacker->moveToThread(workerThread);
    workerThread->start();

    connect(this, SIGNAL(stackImages(int,int)), stacker, SLOT(process(int,int)));
    connect(stacker, SIGNAL(updateProgress(QString,int)), this, SLOT(printProgressBar(QString,int)));
    connect(stacker, SIGNAL(finished(cv::Mat,QString)), this, SLOT(stackingFinished(cv::Mat,QString)));
    connect(this, SIGNAL(detectStars(QString,int)), stacker, SLOT(detectStars(QString,int)));
    connect(stacker, SIGNAL(doneDetectingStars(int)), this, SLOT(starDetectionFinished(int)));
    connect(stacker, SIGNAL(processingError(QString)), this, SLOT(stackingError(QString)));
}

OSS::~OSS()
{
    workerThread->quit();
    workerThread->wait();
    delete workerThread;
    delete stacker;
}

void OSS::printProgressBar(QString message, int percentage)
{
    QString p = QString(" %1% [").arg(QString::number(percentage).rightJustified(3, ' '));

    float progress = percentage / 100.0f;
    int pos = progress * progressBarWidth;
    for (int i = 0; i < progressBarWidth; i++) {
        if (i < pos) p += "=";
        else if (i == pos) p += ">";
        else p += " ";
    }

    p += QString("] %1").arg(message);
    if (p.size() > maxMessageLength) {
        maxMessageLength = p.size();
    }

    cout << p.leftJustified(maxMessageLength, ' ').toUtf8().constData() << "\r" << flush;
}

void suppressDebugOutput(QtMsgType, const QMessageLogContext &, const QString &) {

}

void OSS::run()
{
    qInstallMessageHandler(suppressDebugOutput);
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.setApplicationDescription("Multi-platform deep-sky stacker for astrophotography.");
    parser.addHelpOption();

    QCommandLineOption listOption("f", tr("Image list JSON file."), "list");
    parser.addOption(listOption);

    QCommandLineOption detectOption("s", tr("Detect and print the number of stars in the reference image "
            "with the given threshold, then exit. Ignores all other options except -f and -t."));
    parser.addOption(detectOption);

    QCommandLineOption outputOption("o", tr("Output image file."), "output");
    parser.addOption(outputOption);

    QCommandLineOption thresholdOption("t", tr("Star detection threshold (1-100). Default: 20"), "threshold", "20");
    parser.addOption(thresholdOption);

    QCommandLineOption threadsOption("j", tr("Number of processing threads. Default: 1"), "threads", "1");
    parser.addOption(threadsOption);

    parser.process(QCoreApplication::arguments());

    if (!parser.isSet(listOption)) {
        printf("Error: Must specify an image list\n\n");
        parser.showHelp(1);
        return;
    }
    QString listFile = parser.value(listOption);

    bool intParsingOk = true;
    int threshold = parser.value(thresholdOption).toInt(&intParsingOk);
    if (!intParsingOk || threshold < 1 || threshold > 100) {
        printf("Error: Thread count argument must be an integer between 1 and 100\n");
        parser.showHelp(1);
        return;
    }

    int err = 0;
    vector<ImageRecord *> records = loadImageList(listFile, &err);
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
        if (record->reference)
            referenceSet = true;
    }

    for (ImageRecord *record : records) {
        if (!record->checked)
            continue;

        QString filename = record->filename;

        switch(record->type) {
        case ImageRecord::LIGHT:
            if (!referenceSet || record->reference) {
                ref = filename;
                referenceSet = true;
            } else {
                lights.append(filename);
            }
            break;
        case ImageRecord::DARK:
            darks.append(filename);
            stacker->setUseDarks(true);
            break;
        case ImageRecord::DARK_FLAT:
            darkFlats.append(filename);
            stacker->setUseDarkFlats(true);
            break;
        case ImageRecord::FLAT:
            flats.append(filename);
            stacker->setUseFlats(true);
            break;
        case ImageRecord::BIAS:
            bias.append(filename);
            stacker->setUseBias(true);
            break;
        }
    }

    if (parser.isSet(detectOption)) {
        emit detectStars(ref, threshold);
        return;
    }

    if (!parser.isSet(outputOption)) {
        printf("Error: Must specify an output image\n");
        parser.showHelp(1);
        return;
    }
    outputFileName = parser.value(outputOption);
    QRegularExpression regex("\\.tif$");
    if (!regex.match(outputFileName).hasMatch()) {
        outputFileName += ".tif";
    }

    intParsingOk = true;
    int threads = parser.value(threadsOption).toInt(&intParsingOk);
    if (!intParsingOk || threads < 1) {
        printf("Error: Thread count argument must be a positive non-zero integer\n");
        parser.showHelp(1);
        return;
    }

    stacker->setRefImageFileName(ref);
    stacker->setTargetImageFileNames(lights);
    stacker->setDarkFrameFileNames(darks);
    stacker->setDarkFlatFrameFileNames(darkFlats);
    stacker->setFlatFrameFileNames(flats);
    stacker->setBiasFrameFileNames(bias);

    emit stackImages(threshold, threads);
}

void OSS::stackingFinished(cv::Mat image, QString message)
{
    printProgressBar(message, 100);
    printf("\n");
    try {
        cv::imwrite(outputFileName.toUtf8().constData(), image);
    } catch (exception) {
        printf("Error: Couldn't write resulting image\n");
        QCoreApplication::exit(1);
        return;
    }
    printf("File saved to %s\n", outputFileName.toUtf8().constData());
    emit finished();
}

void OSS::starDetectionFinished(int stars)
{
    printf("%d\n", stars);
    emit finished();
}

void OSS::stackingError(QString message)
{
    printf("\nError: %s\n", message.toUtf8().constData());
    QCoreApplication::exit(1);
}
