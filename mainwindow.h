#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <opencv2/core/core.hpp>
#include <imagestacker.h>
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void stackImages(QString refImage, QStringList targetImages);

public slots:
    void finishedStacking(cv::Mat image);
    void setProgressBar(int value);

private slots:
    void handleButtonRefImage();
    void handleButtonTargetImages();
    void handleButtonStack();
    void setImage(QString filename);

private:
    Ui::MainWindow *ui;
    QString refImageFileName;
    QStringList targetImageFileNames;
    QString saveFilePath;
    QThread *workerThread;

    ImageStacker *stacker;

    cv::Mat workingImage;
    cv::Mat refImage;
    cv::Mat finalImage;

    QDir selectedDir;
};

#endif // MAINWINDOW_H
