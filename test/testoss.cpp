#include "testoss.h"
#include <libstacker/imagestacker.h>
#include <QThread>

using namespace openskystacker;

TestOSS::TestOSS(QString dir) : samplesPath(dir) {}

void TestOSS::initTestCase()
{
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
    QSignalSpy stackSpy(stacker, SIGNAL(finished(cv::Mat,QString)));
    QSignalSpy errorSpy(stacker, SIGNAL(processingError(QString)));

    int err = 0;
    std::vector<ImageRecord *> records = loadImageList(jsonfile, &err);
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

    stacker->setRefImageFileName(ref);
    stacker->setTargetImageFileNames(lights);
    stacker->setDarkFrameFileNames(darks);
    stacker->setDarkFlatFrameFileNames(darkFlats);
    stacker->setFlatFrameFileNames(flats);
    stacker->setBiasFrameFileNames(bias);

    qInstallMessageHandler(suppressDebugOutput);
    try {
        stacker->process(threshold, 1);
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

// QTEST_GUILESS_MAIN(TestOSS)

// HACKS
int main(int argc, char *argv[]) {
    QStringList args;
    for (int i = 0; i < argc; i++) {
        args << argv[i];
    }

    int dirIndex = -1;
    for (int i = 0; i < args.length(); i++) {
        QString arg = args.at(i);
        if (arg == "-d")
            dirIndex = i;
    }

    if (dirIndex < 0) {
        printf("Error: Must specify sample images directory with '-d <dir>'\n");
        return 1;
    }

    QString dir = args.at(dirIndex + 1);
    args.removeAt(dirIndex);
    args.removeAt(dirIndex);
    return QTest::qExec(new TestOSS(dir), args);
}
