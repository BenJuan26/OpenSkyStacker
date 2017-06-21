#include <QtTest/QtTest>

class TestOSS: public QObject
{
    Q_OBJECT
private slots:
    void testUseDarks();
};

QTEST_MAIN(TestOSS)
//#include "moc_testoss.cpp"