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
    void testStackRawImages();
};

}

#endif // TESTOSS_H
