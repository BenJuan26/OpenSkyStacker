#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

private:
    Ui::MainWindow *ui;
    QString refImageFileName;
    QStringList targetImageFileNames;

    cv::Mat generateAlignedImage(cv::Mat ref, cv::Mat target);

    cv::Mat workingImage;
    cv::Mat refImage;
};

#endif // MAINWINDOW_H
