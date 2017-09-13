#include "testoss.h"
#include "processing/imagestacker.h"
#include <QtTest/QtTest>

void TestOSS::testUseDarks()
{
    ImageStacker stacker;
    stacker.use_darks_ = true;

    QCOMPARE(stacker.use_darks_, true);
}

QTEST_MAIN(TestOSS)