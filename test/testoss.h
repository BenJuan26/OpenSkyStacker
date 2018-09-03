#ifndef TESTOSS_H
#define TESTOSS_H

#include <QObject>
#include <QtTest/QtTest>

namespace openskystacker {

//! Main class for testing.
class TestOSS: public QObject
{
    Q_OBJECT
    QString samplesPath;

private slots:
    void initTestCase();

    void testDetectStars();
    void testDetectStars_data();

    void testStackImages();
    void testStackImages_data();

    void testAdjoiningPixel();

public:
    TestOSS(QString);
};

}

#endif // TESTOSS_H
