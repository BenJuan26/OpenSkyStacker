#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <opencv2/core/core.hpp>
#include <processing/imagestacker.h>
#include <QThread>
#include "model/imagetablemodel.h"

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
    void readQImage(QString filename);

public slots:
    void finishedStacking(cv::Mat image);
    void updateProgress(QString message, int percentComplete);
    void clearProgress(QString message);
    void showTableContextMenu(QPoint pos);
    void setFrameAsReference();
    void removeImages();
    void imageSelectionChanged();
    void setImage(QImage image);
    void checkImages();
    void uncheckImages();

private slots:
    //void handleButtonRefImage();
    void handleButtonLightFrames();
    void handleButtonStack();
    void handleButtonDarkFrames();
    void handleButtonDarkFlatFrames();
    void handleButtonFlatFrames();
    void handleButtonBiasFrames();

private:
    void setFileImage(QString filename);
    void setMemImage(QImage image);
    void clearReferenceFrame();
    QImage Mat2QImage(const cv::Mat &src);
    Ui::MainWindow *ui;

    QThread *workerThread;

    ImageStacker *stacker;

    QDir selectedDir;
    QStringList imageFileFilter;
    ImageTableModel tableModel;
};

#endif // MAINWINDOW_H
