#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Mat im1 = imread("F:\\Astro\\Samples\\M42\\example1.jpg");
    Mat im2 = imread("F:\\Astro\\Samples\\M42\\example1.jpg");

    // Convert images to gray scale;
    Mat im1_gray, im2_gray;
    cvtColor(im1, im1_gray, CV_BGR2GRAY);
    cvtColor(im2, im2_gray, CV_BGR2GRAY);

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
    int number_of_iterations = 5000;

    // Specify the threshold of the increment
    // in the correlation coefficient between two iterations
    double termination_eps = 1e-10;

    // Define termination criteria
    TermCriteria criteria (TermCriteria::COUNT+TermCriteria::EPS, number_of_iterations, termination_eps);

    // Run the ECC algorithm. The results are stored in warp_matrix.
    findTransformECC(
                     im1_gray,
                     im2_gray,
                     warp_matrix,
                     warp_mode,
                     criteria
                 );

    // Storage for warped image.
    Mat im2_aligned;

    if (warp_mode != MOTION_HOMOGRAPHY)
        // Use warpAffine for Translation, Euclidean and Affine
        warpAffine(im2, im2_aligned, warp_matrix, im1.size(), INTER_LINEAR + WARP_INVERSE_MAP);
    else
        // Use warpPerspective for Homography
        warpPerspective (im2, im2_aligned, warp_matrix, im1.size(),INTER_LINEAR + WARP_INVERSE_MAP);

    // Show final result
    //imshow("Image 1", im1);
    //imshow("Image 2", im2);
    //imshow("Image 2 Aligned", im2_aligned);
    imwrite("F:\\Astro\\Samples\\M42\\aligned.jpg", im2_aligned);
}

MainWindow::~MainWindow()
{
    delete ui;
}
