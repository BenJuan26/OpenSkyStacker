#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->buttonSelectRefImage, SIGNAL (released()), this, SLOT (handleButtonRefImage()));
    connect(ui->buttonSelectTargetImages, SIGNAL (released()), this, SLOT (handleButtonTargetImages()));
    connect(ui->buttonStack, SIGNAL (released()), this, SLOT (handleButtonStack()));
}

cv::Mat MainWindow::generateAlignedImage(Mat ref, Mat target) {
    // Convert images to gray scale;
    Mat ref_gray, target_gray;
    cvtColor(ref, ref_gray, CV_BGR2GRAY);
    cvtColor(target, target_gray, CV_BGR2GRAY);

    // Define the motion model
    const int warp_mode = MOTION_EUCLIDEAN;

    // Set a 2x3 or 3x3 warp matrix depending on the motion model.
    Mat warp_matrix;

    // Initialize the matrix to identity
    if ( warp_mode == MOTION_HOMOGRAPHY )
        warp_matrix = Mat::eye(3, 3, CV_32F);
    else
        warp_matrix = Mat::eye(2, 3, CV_32F);

    // Specify the number of iterations.
    int number_of_iterations = 500;

    // Specify the threshold of the increment
    // in the correlation coefficient between two iterations
    double termination_eps = 1e-8;

    // Define termination criteria
    TermCriteria criteria (TermCriteria::COUNT+TermCriteria::EPS, number_of_iterations, termination_eps);

    // Run the ECC algorithm. The results are stored in warp_matrix.
    findTransformECC(
                     ref_gray,
                     target_gray,
                     warp_matrix,
                     warp_mode,
                     criteria
                 );

    // Storage for warped image.
    Mat target_aligned;

    if (warp_mode != MOTION_HOMOGRAPHY)
        // Use warpAffine for Translation, Euclidean and Affine
        warpAffine(target, target_aligned, warp_matrix, ref.size(), INTER_LINEAR + WARP_INVERSE_MAP);
    else
        // Use warpPerspective for Homography
        warpPerspective (target, target_aligned, warp_matrix, ref.size(),INTER_LINEAR + WARP_INVERSE_MAP);

    return target_aligned;
}

void MainWindow::handleButtonStack() {

    QString saveFilePath = QFileDialog::getSaveFileName(
                this, "Select Output Image", selectedDir.absolutePath(), "TIFF Image (*.tif)");

    if (saveFilePath.isEmpty()) {
        qDebug() << "No output file selected. Cancelling.";
        return;
    }

    Mat refImage = imread(refImageFileName.toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
    refImage.convertTo(workingImage, CV_16UC3, 256);

    for (int k = 0; k < targetImageFileNames.length(); k++) {
        Mat targetImage = imread(targetImageFileNames.at(k).toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
        //targetImage.convertTo(targetImage, CV_32F);

        Mat targetAligned = generateAlignedImage(refImage, targetImage);
        qDebug() << "Aligned image " << k+1 << " of " << targetImageFileNames.length();
        ui->progressBar->setValue(k*2 + 1);

        workingImage = averageImages32F(workingImage, targetAligned);
        qDebug() << "Stacked image " << k+1 << " of " << targetImageFileNames.length();
        ui->progressBar->setValue(k*2 + 2);
    }

    imwrite(saveFilePath.toUtf8().constData(), workingImage);
    qDebug() << "Done stacking";
}

cv::Mat MainWindow::averageImages32F(cv::Mat img1, cv::Mat img2) {
    Mat result = Mat(img1.rows, img1.cols, CV_16UC3);

    // TODO: Making some BRUTAL assumptions here.
    // Should be checking the depth of both the source and target images!
    for(int i = 0; i < img1.cols; i++) {
        for(int j = 0; j < img1.rows * 3; j++) {
            int b1 = img1.at<unsigned short>(img1.cols * j + i);
            int g1 = img1.at<unsigned short>(img1.cols * j + i + 1);
            int r1 = img1.at<unsigned short>(img1.cols * j + i + 2);

            int b2 = img2.at<unsigned char>(img1.cols * j + i) * 256;
            int g2 = img2.at<unsigned char>(img1.cols * j + i + 1) * 256;
            int r2 = img2.at<unsigned char>(img1.cols * j + i + 2) * 256;

            result.at<unsigned short>(img1.cols * j + i) = (b1 + b2) / 2;
            result.at<unsigned short>(img1.cols * j + i + 1) = (g1 + g2) / 2;
            result.at<unsigned short>(img1.cols * j + i + 2) = (r1 + r2) / 2;
        }
    }

    return result;
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
    ui->progressBar->setMaximum(targetImageFileNames.length() * 2);
}

MainWindow::~MainWindow()
{
    delete ui;
}
