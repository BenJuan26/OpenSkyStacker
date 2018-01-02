#ifndef TESTOSS_H
#define TESTOSS_H

#include "processing/imagestacker.h"

#include <QObject>
#include <QThread>
#include <QtTest/QtTest>

namespace openskystacker {

//! Main class for testing.
class TestOSS: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void testDetectStars();
    void testDetectStars_data();

    void testStackImages();
    void testStackImages_data();

private:
    QString appPath;
    QString samplesPath;
};

}

#endif // TESTOSS_H
