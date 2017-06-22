#include "testoss.h"
#include "processing/imagestacker.h"
#include <QtTest/QtTest>

void TestOSS::testUseDarks()
{
    ImageStacker stacker;
    stacker.useDarks = true;

    QCOMPARE(stacker.useDarks, true);
}

QTEST_MAIN(TestOSS)