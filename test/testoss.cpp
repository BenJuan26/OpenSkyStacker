#include "processing/imagestacker.h"
#include <QtTest/QtTest>

class TestOSS: public QObject
{
    Q_OBJECT
private slots:
    void testUseDarks();
};

void TestOSS::testUseDarks()
{
    ImageStacker stacker;
    stacker.useDarks = true;

    QCOMPARE(stacker.useDarks, true);
}

QTEST_MAIN(TestOSS)
#include "testoss.moc"