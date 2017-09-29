#include "testoss.h"
#include "processing/imagestacker.h"
#include <QtTest/QtTest>

using namespace openskystacker;

void TestOSS::testUseDarks()
{
    ImageStacker stacker;
    stacker.SetUseDarks(true);

    QCOMPARE(stacker.use_darks_, true);
}

QTEST_MAIN(TestOSS)
