#include "testoss.h"
#include <libstacker/imagestacker.h>
#include <libstacker/stardetector.h>
#include <libstacker/util.h>
#include <adjoiningpixel.h>
#include <QThread>

using namespace openskystacker;

TestOSS::TestOSS(QString dir) : samplesPath(dir) {}

void TestOSS::drawFakeStar(cv::Mat image, int x, int y)
{
    cv::Vec3f vec5(1.0, 1.0, 1.0);
    cv::Vec3f vec4(0.8, 0.8, 0.8);
    cv::Vec3f vec3(0.6, 0.6, 0.6);
    cv::Vec3f vec2(0.4, 0.4, 0.4);

    image.at<cv::Vec3f>(y, x) = vec5;
    image.at<cv::Vec3f>(y, x-1) = vec5;
    image.at<cv::Vec3f>(y, x+1) = vec5;
    image.at<cv::Vec3f>(y-1, x) = vec5;
    image.at<cv::Vec3f>(y+1, x) = vec5;

    image.at<cv::Vec3f>(y-2, x) = vec4;
    image.at<cv::Vec3f>(y+2, x) = vec4;
    image.at<cv::Vec3f>(y, x+2) = vec4;
    image.at<cv::Vec3f>(y, x-2) = vec4;
    image.at<cv::Vec3f>(y-1, x-1) = vec4;
    image.at<cv::Vec3f>(y-1, x+1) = vec4;
    image.at<cv::Vec3f>(y+1, x+1) = vec4;
    image.at<cv::Vec3f>(y+1, x-1) = vec4;

    image.at<cv::Vec3f>(y-3, x) = vec3;
    image.at<cv::Vec3f>(y+3, x) = vec3;
    image.at<cv::Vec3f>(y, x-3) = vec3;
    image.at<cv::Vec3f>(y, x+3) = vec3;
    image.at<cv::Vec3f>(y-2, x+1) = vec3;
    image.at<cv::Vec3f>(y-1, x+2) = vec3;
    image.at<cv::Vec3f>(y+1, x+2) = vec3;
    image.at<cv::Vec3f>(y+2, x+1) = vec3;
    image.at<cv::Vec3f>(y+2, x-1) = vec3;
    image.at<cv::Vec3f>(y+1, x-2) = vec3;
    image.at<cv::Vec3f>(y-1, x-2) = vec3;
    image.at<cv::Vec3f>(y-2, x-1) = vec3;

    image.at<cv::Vec3f>(y-2, x-2) = vec2;
    image.at<cv::Vec3f>(y-2, x+2) = vec2;
    image.at<cv::Vec3f>(y+2, x-2) = vec2;
    image.at<cv::Vec3f>(y+2, x+2) = vec2;
}

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

void TestOSS::testAdjoiningPixel()
{
    AdjoiningPixel ap;
    ap.addPixel(Pixel(5, 10, 0.5));

    QCOMPARE(ap.getPeak().x, 5);
    QCOMPARE(ap.getPeak().y, 10);
    QCOMPARE(ap.getPeak().value, 0.5);
    QCOMPARE(ap.getPeakValue(), 0.5);

    std::vector<Pixel> pixVector;
    pixVector.push_back(Pixel(100, 100, 0.5));
    pixVector.push_back(Pixel(20, 20, 0.25));

    ap.setPixels(pixVector);
    QCOMPARE(ap.getGravityCenter().x, 73);
    QCOMPARE(ap.getGravityCenter().y, 73);

    pixVector.clear();
    pixVector.push_back(Pixel(75, 75, 1.0));
    pixVector.push_back(Pixel(76, 75, 1.0));
    pixVector.push_back(Pixel(74, 75, 1.0));
    pixVector.push_back(Pixel(75, 76, 1.0));
    pixVector.push_back(Pixel(74, 75, 1.0));
    pixVector.push_back(Pixel(74, 74, 0.5));
    pixVector.push_back(Pixel(74, 76, 0.5));
    pixVector.push_back(Pixel(76, 76, 0.5));
    pixVector.push_back(Pixel(76, 74, 0.5));
    pixVector.push_back(Pixel(74, 73, 0.25));
    pixVector.push_back(Pixel(73, 74, 0.25));
    pixVector.push_back(Pixel(73, 76, 0.25));
    pixVector.push_back(Pixel(77, 74, 0.25));
    pixVector.push_back(Pixel(77, 76, 0.25));
    pixVector.push_back(Pixel(76, 77, 0.25));

    AdjoiningPixel ap2;
    ap2.setPixels(pixVector);
    QVERIFY(ap2 > ap);
    QVERIFY(ap < ap2);

    std::vector<AdjoiningPixel> deblended = ap2.deblend(10);
    QCOMPARE(static_cast<int>(deblended.size()), 1);

    Star star = deblended.at(0).createStar();
    QCOMPARE(star.peak, 1.0);
    QCOMPARE(star.x, 75);
    QCOMPARE(star.y, 75);
}

void TestOSS::testStarDetector()
{
    cv::Mat image(100, 100, CV_32FC3);
    image = cv::Scalar(0.1, 0.1, 0.1);

    drawFakeStar(image, 35, 40);
    drawFakeStar(image, 60, 80);

    StarDetector sd;
    std::vector<Star> stars = sd.getStars(image);

    QCOMPARE(static_cast<int>(stars.size()), 0);

    stars = sd.getStars(image, 5);

    QCOMPARE(static_cast<int>(stars.size()), 2);
    QCOMPARE(stars.at(0).x, 35);
    QCOMPARE(stars.at(0).y, 40);
    QCOMPARE(stars.at(1).x, 60);
    QCOMPARE(stars.at(1).y, 80);

    sd.drawDetectedStars("teststars.jpg", 100, 100, -1, stars);

    cv::Mat bg = sd.generateSkyBackground(image);
    double min, max;
    cv::minMaxLoc(bg, &min, &max);

    QVERIFY(max < 0.2);
}

void TestOSS::testGetBayerMatrix()
{
    QString path = samplesPath + "/Raw/Lights/DSC_4494.NEF";
    cv::Mat bayer = getBayerMatrix(path);

    QCOMPARE(bayer.rows, 4352);
    QCOMPARE(bayer.cols, 2868);
    QCOMPARE(bayer.type(), CV_16UC1);
}

void TestOSS::testGetImageRecord_data() {
    QTest::addColumn<QString>("filename");
    QTest::addColumn<float>("iso");
    QTest::addColumn<float>("shutter");
    QTest::addColumn<long>("timestamp");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");


    QTest::newRow("Raw M42") << samplesPath + "/Raw/Lights/DSC_4494.NEF" << 800.f << 60.f
            << 1168396735L << 4320 << 2868;
    QTest::newRow("JPEG M42") << samplesPath + "/JPEG/Lights/DSC_4494.jpg" << 800.f << 60.f
            << 1489673463L << 0 << 0;
    QTest::newRow("FITS Heart and Soul")
            << samplesPath + "/FITS/HeartAndSoul_Light_Ha_300sec_1x1_frame1_-15.1C.fit"
            << -1.f << 300.f << 1503136924L << 4656 << 3520;
}

void TestOSS::testGetImageRecord()
{
    QFETCH(QString, filename);
    QFETCH(float, iso);
    QFETCH(float, shutter);
    QFETCH(long, timestamp);
    QFETCH(int, width);
    QFETCH(int, height);

    ImageRecord *record = getImageRecord(filename);

    QCOMPARE(record->iso, iso);
    QCOMPARE(record->shutter, shutter);
    QCOMPARE(record->timestamp, timestamp);
    QCOMPARE(record->width, width);
    QCOMPARE(record->height, height);

    delete record;
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
