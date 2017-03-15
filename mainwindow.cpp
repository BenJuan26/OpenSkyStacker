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

    Mat refImage = imread(refImageFileName.toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
    workingImage = Mat(refImage);

    for (int k = 0; k < targetImageFileNames.length(); k++) {
        Mat targetImage = imread(targetImageFileNames.at(k).toUtf8().constData(), CV_LOAD_IMAGE_COLOR);

        Mat targetAligned = generateAlignedImage(refImage, targetImage);

        unsigned char *targetInput = (unsigned char*)(targetAligned.data);
        unsigned char *workingInput = (unsigned char*)(workingImage.data);

        for(int i = 0; i < workingImage.cols; i++) {
            for(int j = 0; j < workingImage.rows * 3; j++) {
                int b1 = workingInput[workingImage.cols * j + i ] ;
                int g1 = workingInput[workingImage.cols * j + i + 1];
                int r1 = workingInput[workingImage.cols * j + i + 2];

                int b2 = targetInput[workingImage.cols * j + i ] ;
                int g2 = targetInput[workingImage.cols * j + i + 1];
                int r2 = targetInput[workingImage.cols * j + i + 2];

                workingInput[workingImage.cols * j + i ] = (b1 + b2) / 2;
                workingInput[workingImage.cols * j + i + 1] = (g1 + g2) / 2;
                workingInput[workingImage.cols * j + i + 2] = (r1 + r2) / 2;
            }
        }

        imwrite("/Users/Ben/Downloads/stacked.png", workingImage);
        qDebug() << "Done stacking";
    }
}

void MainWindow::handleButtonRefImage() {
    QFileDialog dialog(this);
    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("All images (*.jpg *.jpeg *.png)");

    if (!dialog.exec()) {
        QMessageBox box;
        box.setText("Error selecting image");
        box.exec();

        return;
    }

    refImageFileName = dialog.selectedFiles().at(0);
    qDebug() <<  refImageFileName;

    ui->buttonSelectTargetImages->setEnabled(true);
}

void MainWindow::handleButtonTargetImages() {
    QFileDialog dialog(this);
    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter("All images (*.jpg *.jpeg *.png)");

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

MainWindow::~MainWindow()
{
    delete ui;
}
