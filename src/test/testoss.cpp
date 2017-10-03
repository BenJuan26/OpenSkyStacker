#include "testoss.h"

using namespace openskystacker;

void TestOSS::testUseDarks()
{
    ImageStacker stacker;
    stacker.SetUseDarks(true);

    QCOMPARE(stacker.use_darks_, true);
}

void TestOSS::testDetectStars()
{
    QString binPath = qApp->applicationDirPath();

    ImageStacker *stacker = new ImageStacker();
    QSignalSpy spy(stacker, SIGNAL(doneDetectingStars(int)));

    int err = 0;
    std::vector<ImageRecord *> records = ImageStacker::LoadImageList(binPath + "/../src/images/samples/detect.json", &err);
    QCOMPARE(err, 0);

    ImageRecord *record = records.at(0);

    stacker->detectStars(record->GetFilename(), 20);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> list = spy.takeFirst();
    int result = list.at(0).toInt();
    QVERIFY(result > 60);
}

QTEST_MAIN(TestOSS)
