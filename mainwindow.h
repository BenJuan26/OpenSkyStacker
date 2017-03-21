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
    void stackImages();

public slots:
    void finishedStacking(cv::Mat image);

private slots:
    void handleButtonRefImage();
    void handleButtonTargetImages();
    void handleButtonStack();
    void handleButtonDarkFrames();
    void handleButtonDarkFlatFrames();
    void handleButtonFlatFrames();
    void setImage(QString filename);

private:
    Ui::MainWindow *ui;

    QThread *workerThread;

    ImageStacker *stacker;

    QDir selectedDir;
};

#endif // MAINWINDOW_H
