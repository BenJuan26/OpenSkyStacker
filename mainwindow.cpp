#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processingdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsPixMapItem>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // cv::Mat can't be passed through a signal without this declaration
    qRegisterMetaType<cv::Mat>("cv::Mat");

    stacker = new ImageStacker();
    workerThread = new QThread();

    stacker->moveToThread(workerThread);
    workerThread->start();

    connect(ui->buttonSelectRefImage, SIGNAL (released()), this, SLOT (handleButtonRefImage()));
    connect(ui->buttonSelectTargetImages, SIGNAL (released()), this, SLOT (handleButtonTargetImages()));
    connect(ui->buttonStack, SIGNAL (released()), this, SLOT (handleButtonStack()));
    connect(this, SIGNAL (stackImages(QString,QStringList)), stacker, SLOT(process(QString, QStringList)));
    connect(stacker, SIGNAL(finished(cv::Mat)), this, SLOT(finishedStacking(cv::Mat)));
    //connect(stacker, SIGNAL(updateProgressBar(int)), this, SLOT(setProgressBar(int)));
}

void MainWindow::finishedStacking(Mat image) {
    imwrite(saveFilePath.toUtf8().constData(), image);
    setImage(saveFilePath);
    qDebug() << "Done stacking";
}

void MainWindow::handleButtonStack() {

    saveFilePath = QFileDialog::getSaveFileName(
                this, "Select Output Image", selectedDir.absolutePath(), "TIFF Image (*.tif)");

    if (saveFilePath.isEmpty()) {
        qDebug() << "No output file selected. Cancelling.";
        return;
    }

    // asynchronously trigger the processing
    emit stackImages(refImageFileName, targetImageFileNames);

    ProcessingDialog *dialog = new ProcessingDialog(this);
    connect(stacker, SIGNAL(updateProgressBar(QString,int)), dialog, SLOT(updateProgress(QString,int)));
    connect(stacker, SIGNAL(finishedDialog(QString)), dialog, SLOT(complete(QString)));

    if (!dialog->exec()) {
        qDebug() << "Cancelling...";
        stacker->cancel = true;
    }
}

void MainWindow::handleButtonRefImage() {
    QFileDialog dialog(this);
    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("All images (*.jpg *.jpeg *.png *.tif)");

    if (!dialog.exec()) {
        QMessageBox box;
        box.setText("Error selecting image");
        box.exec();

        return;
    }

    refImageFileName = dialog.selectedFiles().at(0);

    QFileInfo info(refImageFileName);
    selectedDir = QDir(info.absoluteFilePath());
    setImage(refImageFileName);
    qDebug() <<  refImageFileName;

    ui->buttonSelectTargetImages->setEnabled(true);
}

void MainWindow::handleButtonTargetImages() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter("All images (*.jpg *.jpeg *.png *.tif)");

    if (!dialog.exec()) {
        QMessageBox box;
        box.setText("Error selecting images");
        box.exec();

        return;
    }

    targetImageFileNames = dialog.selectedFiles();

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        qDebug() << targetImageFileNames.at(i);
    }

    ui->buttonStack->setEnabled(true);
}

void MainWindow::setImage(QString filename) {
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap(filename));
    ui->imageHolder->setScene(scene);
    ui->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

MainWindow::~MainWindow()
{
    delete ui;
    workerThread->quit();
    workerThread->wait();
    delete workerThread;
    delete stacker;
}
