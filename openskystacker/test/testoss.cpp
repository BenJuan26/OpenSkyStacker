#include "testoss.h"

using namespace openskystacker;

void TestOSS::initTestCase()
{
    appPath = qApp->applicationDirPath();
    samplesPath = appPath + "/../src/images/samples";
    qRegisterMetaType<cv::Mat>("cv::Mat");
}

void TestOSS::testDetectStars()
{
    QFETCH(QString, filename);
    QFETCH(int, numstars);

    ImageStacker *stacker = new ImageStacker();
    QSignalSpy spy(stacker, SIGNAL(doneDetectingStars(int)));

    stacker->detectStars(filename, 20);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> list = spy.takeFirst();
    int result = list.at(0).toInt();
    QVERIFY(result > numstars);
}

void TestOSS::testDetectStars_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<int>("threshold");
    QTest::addColumn<int>("numstars");

    QTest::newRow("Raw M42") << samplesPath + "/Raw/Lights/DSC_4494.NEF" << 20 << 60;
    QTest::newRow("FITS Heart and Soul")
            << samplesPath + "/FITS/HeartAndSoul_Light_Ha_300sec_1x1_frame1_-15.1C.fit" << 50 << 200;
}

void TestOSS::testStackImages_data()
{
    QTest::addColumn<QString>("jsonfile");
    QTest::addColumn<int>("threshold");

    QTest::newRow("Raw M42") << samplesPath + "/Raw/all.json" << 20;
    QTest::newRow("JPEG M42") << samplesPath + "/JPEG/all.json" << 20;
    QTest::newRow("FITS Heart and Soul") << samplesPath + "/FITS/all.json" << 50;
}

void suppressDebugOutput(QtMsgType, const QMessageLogContext &, const QString &) {

}

void TestOSS::testStackImages()
{
    QFETCH(QString, jsonfile);
    QFETCH(int, threshold);

    ImageStacker *stacker = new ImageStacker();
    QSignalSpy stackSpy(stacker, SIGNAL(Finished(cv::Mat,QString)));
    QSignalSpy errorSpy(stacker, SIGNAL(ProcessingError(QString)));

    int err = 0;
    std::vector<ImageRecord *> records = LoadImageList(jsonfile, &err);
    QCOMPARE(err, 0);

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
    try {
        stacker->Process(threshold, 1);
    } catch (std::exception) {
        QFAIL("Exception thrown");
    }

    qInstallMessageHandler(0);

    if (errorSpy.count() > 0) {
        QList<QVariant> list = errorSpy.takeFirst();
        QString result = list.at(0).toString();
        QFAIL(result.toUtf8().constData());
    }
    QCOMPARE(stackSpy.count(), 1);
}

QTEST_GUILESS_MAIN(TestOSS)
