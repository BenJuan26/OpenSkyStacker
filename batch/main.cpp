#include "batch.h"
#include <libstacker/imagestacker.h>

#include <QtCore>

#include <stdio.h>

using std::printf;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    OSSBatch *batch = new OSSBatch(argc, argv, &a);
    QObject::connect(batch, SIGNAL(Finished()), &a, SLOT(quit()));

    QTimer::singleShot(0, batch, SLOT(Run()));

    return a.exec();
}
