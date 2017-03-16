#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <opencv2/core/core.hpp>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void handleButtonRefImage();
    void handleButtonTargetImages();
    void handleButtonStack();
    void setImage(QString filename);

private:
    Ui::MainWindow *ui;
    QString refImageFileName;
    QStringList targetImageFileNames;

    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);
    cv::Mat averageImages32F(cv::Mat img1, cv::Mat img2);

    cv::Mat workingImage;
    cv::Mat refImage;

    QDir selectedDir;
};

#endif // MAINWINDOW_H
