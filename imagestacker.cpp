#include "imagestacker.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <QDebug>
#include <QTime>

using namespace cv;

ImageStacker::ImageStacker(QObject *parent) : QObject(parent)
{

}

void ImageStacker::process(QString refImageFileName, QStringList targetImageFileNames) {

    // TODO: BAD assumption that the source image is 8-bit
    Mat refImage = imread(refImageFileName.toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
    Mat workingImage;
    refImage.convertTo(workingImage, CV_16UC3, 256);

    for (int k = 0; k < targetImageFileNames.length(); k++) {
        Mat targetImage = imread(targetImageFileNames.at(k).toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
        //targetImage.convertTo(targetImage, CV_32F);

        Mat targetAligned = generateAlignedImage(refImage, targetImage);
        qDebug() << "Aligned image " << k+1 << " of " << targetImageFileNames.length();
        emit updateProgressBar(k*2 + 1);

        workingImage = averageImages(workingImage, targetAligned);
        qDebug() << "Stacked image " << k+1 << " of " << targetImageFileNames.length();
        emit updateProgressBar(k*2 + 2);
    }

    emit finished(workingImage);
}

cv::Mat ImageStacker::averageImages(cv::Mat img1, cv::Mat img2) {
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

cv::Mat ImageStacker::generateAlignedImage(Mat ref, Mat target) {
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
    int number_of_iterations = 50;

    // Specify the threshold of the increment
    // in the correlation coefficient between two iterations
    double termination_eps = 1e-6;

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
