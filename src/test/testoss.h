#ifndef TESTOSS_H
#define TESTOSS_H

#include <QObject>

namespace openskystacker {

//! Main class for testing.
class TestOSS: public QObject
{
    Q_OBJECT
private slots:
    void testUseDarks();
};

}

#endif // TESTOSS_H
