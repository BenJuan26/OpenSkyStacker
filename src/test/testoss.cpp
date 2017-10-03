#include "testoss.h"

using namespace openskystacker;

void TestOSS::testDetectStars()
{
    QString binPath = qApp->applicationDirPath();

    ImageStacker *stacker = new ImageStacker();
    QSignalSpy spy(stacker, SIGNAL(doneDetectingStars(int)));

    int err = 0;
    std::vector<ImageRecord *> records = ImageStacker::LoadImageList(binPath + "/../src/images/samples/detect.json", &err);
    QCOMPARE(err, 0);

    ImageRecord *record = records.at(0);

    stacker->detectStars(record->GetFilename(), 20);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> list = spy.takeFirst();
    int result = list.at(0).toInt();
    QVERIFY(result > 60);
}

void suppressDebugOutput(QtMsgType, const QMessageLogContext &, const QString &) {

}

void TestOSS::testStackRawImages()
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    ImageStacker *stacker = new ImageStacker();
    QSignalSpy stackSpy(stacker, SIGNAL(Finished(cv::Mat,QString)));
    QSignalSpy errorSpy(stacker, SIGNAL(ProcessingError(QString)));

    QString binPath = qApp->applicationDirPath();

    int err = 0;
    std::vector<ImageRecord *> records = ImageStacker::LoadImageList(binPath + "/../src/images/samples/Raw/all.json", &err);
    QCOMPARE(err, 0);

    QString ref;
    QStringList lights;
    QStringList darks;
    QStringList darkFlats;
    QStringList flats;
    QStringList bias;
    bool referenceSet = false;
    for (ImageRecord *record : records) {
        if (!record->IsChecked())
            continue;

        QString filename = record->GetFilename();

        switch(record->GetType()) {
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
            stacker->SetUseDarks(true);
            break;
        case ImageRecord::DARK_FLAT:
            darkFlats.append(filename);
            stacker->SetUseDarkFlats(true);
            break;
        case ImageRecord::FLAT:
            flats.append(filename);
            stacker->SetUseFlats(true);
            break;
        case ImageRecord::BIAS:
            bias.append(filename);
            stacker->SetUseBias(true);
            break;
        }
    }

    stacker->SetRefImageFileName(ref);
    stacker->SetTargetImageFileNames(lights);
    stacker->SetDarkFrameFileNames(darks);
    stacker->SetDarkFlatFrameFileNames(darkFlats);
    stacker->SetFlatFrameFileNames(flats);
    stacker->SetBiasFrameFileNames(bias);

    qInstallMessageHandler(suppressDebugOutput);
    stacker->Process();
    qInstallMessageHandler(0);

    if (errorSpy.count() > 0) {
        QList<QVariant> list = errorSpy.takeFirst();
        QString result = list.at(0).toString();
        QFAIL(result.toUtf8().constData());
    }
    QCOMPARE(stackSpy.count(), 1);
}

QTEST_GUILESS_MAIN(TestOSS)
