#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processingdialog.h"
#include "stardetector.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsPixmapItem>
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
    stacker->setBitsPerChannel(ImageStacker::BITS_32);
    workerThread = new QThread();

    stacker->moveToThread(workerThread);
    workerThread->start();

    imageFileFilter << "All files (*)" << "Image files (*.jpg *.jpeg *.png *.tif)";
    imageFileFilter << "Raw image files (*.NEF *.CR2 *.DNG *.RAW)";

    connect(ui->buttonSelectRefImage, SIGNAL (released()), this, SLOT (handleButtonRefImage()));
    connect(ui->buttonSelectTargetImages, SIGNAL (released()), this, SLOT (handleButtonTargetImages()));
    connect(ui->buttonSelectDarkFrames, SIGNAL (released()), this, SLOT (handleButtonDarkFrames()));
    connect(ui->buttonSelectDarkFlatFrames, SIGNAL (released()), this, SLOT (handleButtonDarkFlatFrames()));
    connect(ui->buttonSelectFlatFrames, SIGNAL (released()), this, SLOT (handleButtonFlatFrames()));

    connect(ui->buttonStack, SIGNAL (released()), this, SLOT (handleButtonStack()));
    connect(this, SIGNAL (stackImages()), stacker, SLOT(process()));
    connect(stacker, SIGNAL(finished(cv::Mat)), this, SLOT(finishedStacking(cv::Mat)));
}

void MainWindow::finishedStacking(Mat image) {
    qDebug() << "finished:" << image.at<float>(0) << image.at<float>(1) << image.at<float>(2);
    QString path = stacker->getSaveFilePath();
    imwrite(path.toUtf8().constData(), image);
    setMemImage(Mat2QImage(image));
    qDebug() << "Done stacking";
}

void MainWindow::handleButtonStack() {

    QString saveFilePath = QFileDialog::getSaveFileName(
                this, "Select Output Image", selectedDir.absolutePath(), "TIFF Image (*.tif)");

    if (saveFilePath.isEmpty()) {
        qDebug() << "No output file selected. Cancelling.";
        return;
    }

    stacker->setSaveFilePath(saveFilePath);

    // asynchronously trigger the processing
    emit stackImages();

    ProcessingDialog *dialog = new ProcessingDialog(this);
    connect(stacker, SIGNAL(updateProgress(QString,int)), dialog, SLOT(updateProgress(QString,int)));
    connect(stacker, SIGNAL(finishedDialog(QString)), dialog, SLOT(complete(QString)));

    if (!dialog->exec()) {
        qDebug() << "Cancelling...";
        stacker->cancel = true;
    }
}

void MainWindow::handleButtonRefImage() {
    /*
    QFileDialog dialog(this);
    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QString refImageFileName = dialog.selectedFiles().at(0);
    stacker->setRefImageFileName(refImageFileName);

    QFileInfo info(refImageFileName);
    selectedDir = QDir(info.absoluteFilePath());
    setFileImage(refImageFileName);
    qDebug() << refImageFileName;

    ui->buttonSelectTargetImages->setEnabled(true);
    */




    /*
    Mat image = stacker->readImage("/Users/Ben/Pictures/OpenSkyStacker/M42/Lights/DSC_4494.NEF");
    //Mat image = stacker->readImage("/Users/Ben/Pictures/OpenSkyStacker/Bodes/Lights/Img1761.nef");

    StarDetector sd;

    sd.process(image);
    */
/*
    StarDetector sd;
    sd.test();
    */
/*
    Mat image = stacker->readImage("/Users/Ben/Pictures/OpenSkyStacker/Bodes/Lights/Img1781.nef");
    Mat xfrm = Mat::eye(2, 3, CV_32F);
    xfrm.at<float>(0,0) = 0.999793;
    xfrm.at<float>(0,1) = 0.00236625;
    xfrm.at<float>(0,2) = 237.761;
    xfrm.at<float>(1,0) = -0.00518012;
    xfrm.at<float>(1,1) = 0.999639;
    xfrm.at<float>(1,2) = 62.1213;

    warpAffine(image, image, xfrm, image.size(), INTER_LINEAR + WARP_INVERSE_MAP);
    imwrite("/Users/Ben/Pictures/OpenSkyStacker/transform.tif", image);
    */

    Mat ref = stacker->readImage("F:\\Astro\\Samples\\Bodes\\Lights\\Img1761.nef");
    Mat target = stacker->readImage("F:\\Astro\\Samples\\Bodes\\Lights\\Img1781.nef");

    stacker->generateAlignedImage(ref, target);
}

void MainWindow::handleButtonTargetImages() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList targetImageFileNames = dialog.selectedFiles();
    stacker->setTargetImageFileNames(targetImageFileNames);

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        qDebug() << targetImageFileNames.at(i);
    }

    ui->buttonStack->setEnabled(true);
    ui->buttonSelectDarkFrames->setEnabled(true);
    ui->buttonSelectDarkFlatFrames->setEnabled(true);
    ui->buttonSelectFlatFrames->setEnabled(true);
}

void MainWindow::handleButtonDarkFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList darkFrameFileNames = dialog.selectedFiles();
    stacker->setDarkFrameFileNames(darkFrameFileNames);
    stacker->setUseDarks(true);

    for (int i = 0; i < darkFrameFileNames.length(); i++) {
        qDebug() << darkFrameFileNames.at(i);
    }
}

void MainWindow::handleButtonDarkFlatFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList darkFlatFrameFileNames = dialog.selectedFiles();
    stacker->setDarkFlatFrameFileNames(darkFlatFrameFileNames);
    stacker->setUseDarkFlats(true);

    for (int i = 0; i < darkFlatFrameFileNames.length(); i++) {
        qDebug() << darkFlatFrameFileNames.at(i);
    }
}

void MainWindow::handleButtonFlatFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilters(imageFileFilter);

    if (!dialog.exec()) return;

    QStringList flatFrameFileNames = dialog.selectedFiles();
    stacker->setFlatFrameFileNames(flatFrameFileNames);
    stacker->setUseFlats(true);

    for (int i = 0; i < flatFrameFileNames.length(); i++) {
        qDebug() << flatFrameFileNames.at(i);
    }
}

QImage MainWindow::Mat2QImage(const cv::Mat &src) {
        QImage dest(src.cols, src.rows, QImage::Format_RGB32);
        int r, g, b;

        if (stacker->getBitsPerChannel() == ImageStacker::BITS_16) {
            for(int x = 0; x < src.cols; x++) {
                for(int y = 0; y < src.rows; y++) {

                    Vec<unsigned short,3> pixel = src.at<Vec<unsigned short,3>>(y,x);
                    b = pixel.val[0]/256;
                    g = pixel.val[1]/256;
                    r = pixel.val[2]/256;
                    dest.setPixel(x, y, qRgb(r,g,b));
                }
            }
        }
        else if (stacker->getBitsPerChannel() == ImageStacker::BITS_32) {
            for(int x = 0; x < src.cols; x++) {
                for(int y = 0; y < src.rows; y++) {

                    Vec3f pixel = src.at<Vec3f>(y,x);
                    b = pixel.val[0]*255;
                    g = pixel.val[1]*255;
                    r = pixel.val[2]*255;
                    dest.setPixel(x, y, qRgb(r,g,b));
                }
            }
        }
        return dest;
}


void MainWindow::setFileImage(QString filename) {

    QGraphicsScene* scene = new QGraphicsScene(this);
    cv::Mat image = stacker->readImage(filename);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(Mat2QImage(image)));
    ui->imageHolder->setScene(scene);
    ui->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::setMemImage(QImage image) {
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(image));
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
