#include "ui/mainwindow.h"
#include <QApplication>

// cv::Mat can't be passed through a signal without this declaration
Q_DECLARE_METATYPE(cv::Mat)

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
