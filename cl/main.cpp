#include "cl.h"
#include <libstacker/imagestacker.h>

#include <QtCore>

#include <stdio.h>

using std::printf;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationVersion("0.2.4");
    QCoreApplication::setApplicationName("OpenSkyStacker");

    OSS *oss = new OSS(&a);
    QObject::connect(oss, SIGNAL(Finished()), &a, SLOT(quit()));

    QTimer::singleShot(0, oss, SLOT(Run()));

    return a.exec();
}
