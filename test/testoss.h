#ifndef TESTOSS_H
#define TESTOSS_H

#include <opencv2/core/core.hpp>

#include <QObject>
#include <QtTest/QtTest>

namespace openskystacker {

//! Main class for testing.
class TestOSS: public QObject
{
    Q_OBJECT
    QString samplesPath;

    void drawFakeStar(cv::Mat image, int x, int y);

private slots:
    void initTestCase();

    void testDetectStars();
    void testDetectStars_data();

    void testStackImages();
    void testStackImages_data();

    void testAdjoiningPixel();
    void testStarDetector();

    void testGetBayerMatrix();
    void testGetImageRecord_data();
    void testGetImageRecord();
    void testExifTimeToCTime();
    void testFitsTimeToCTime();

public:
    TestOSS(QString);
};

}

#endif // TESTOSS_H
