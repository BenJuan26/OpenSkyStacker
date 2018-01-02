#include "ui/mainwindow.h"
#include <QApplication>
#include <QTranslator>

// cv::Mat can't be passed through a signal without this declaration
Q_DECLARE_METATYPE(cv::Mat)

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    QTranslator translator;
//    if (!translator.load("openskystacker_es", QCoreApplication::applicationDirPath())) {
//        qDebug() << "translate load failed";
//    }
//    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
