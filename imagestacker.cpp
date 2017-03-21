#include "imagestacker.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <QDebug>
#include <QTime>

using namespace cv;

ImageStacker::ImageStacker(QObject *parent) : QObject(parent)
{
    cancel = false;
}

void ImageStacker::process(QString refImageFileName, QStringList targetImageFileNames) {

    // TODO: BAD assumption that the source image is 8-bit
    Mat refImage = imread(refImageFileName.toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
    Mat workingImage;
    refImage.convertTo(workingImage, CV_16UC3, 256);

    QString message;

    emit updateProgressBar("Starting stacking process...", 0);

    for (int k = 0; k < targetImageFileNames.length() && !cancel; k++) {
        Mat targetImage = imread(targetImageFileNames.at(k).toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
        //targetImage.convertTo(targetImage, CV_32F);

        Mat targetAligned = generateAlignedImage(refImage, targetImage);
        message = "Aligned image " + QString::number(k+1) + " of " + QString::number(targetImageFileNames.length());
        qDebug() << message;
        emit updateProgressBar(message, (k*2 + 1)*100/(targetImageFileNames.length()*2));

        if (cancel) break;

        workingImage = averageImages16U(workingImage, targetAligned);
        message = "Stacked image " + QString::number(k+1) + " of " + QString::number(targetImageFileNames.length());
        qDebug() << message;
        emit updateProgressBar(message, (k*2 + 2)*100/(targetImageFileNames.length()*2));
    }

    if (cancel) {
        qDebug() << "Cancelled.";
    }
    else {
        emit finishedDialog("Stacking completed");
        emit finished(workingImage);
    }
}

cv::Mat ImageStacker::averageImages16U(cv::Mat img1, cv::Mat img2) {
    Mat result = Mat(img1.rows, img1.cols, CV_16UC3);

    for(int i = 0; i < img1.cols; i++) {
        for(int j = 0; j < img1.rows * 3; j++) {

            int b1, g1, r1;
            switch (img1.depth()) {
                case CV_8U: case CV_8S: default:
                    b1 = img1.at<unsigned char>(img1.cols * j + i) * 256;
                    g1 = img1.at<unsigned char>(img1.cols * j + i + 1) * 256;
                    r1 = img1.at<unsigned char>(img1.cols * j + i + 2) * 256;
                    break;
                case CV_16U: case CV_16S:
                    b1 = img1.at<unsigned short>(img1.cols * j + i);
                    g1 = img1.at<unsigned short>(img1.cols * j + i + 1);
                    r1 = img1.at<unsigned short>(img1.cols * j + i + 2);
                    break;
                case CV_32F: case CV_32S:
                    b1 = img1.at<float>(img1.cols * j + i) / 256;
                    g1 = img1.at<float>(img1.cols * j + i + 1) / 256;
                    r1 = img1.at<float>(img1.cols * j + i + 2) / 256;
                    break;
                case CV_64F:
                    b1 = img1.at<double>(img1.cols * j + i) / 65536;
                    g1 = img1.at<double>(img1.cols * j + i + 1) / 65536;
                    r1 = img1.at<double>(img1.cols * j + i + 2) / 65536;
                    break;
            }

            int b2, g2, r2;
            switch (img2.depth()) {
                case CV_8U: case CV_8S: default:
                    b2 = img2.at<unsigned char>(img2.cols * j + i) * 256;
                    g2 = img2.at<unsigned char>(img2.cols * j + i + 1) * 256;
                    r2 = img2.at<unsigned char>(img2.cols * j + i + 2) * 256;
                    break;
                case CV_16U: case CV_16S:
                    b2 = img2.at<unsigned short>(img2.cols * j + i);
                    g2 = img2.at<unsigned short>(img2.cols * j + i + 1);
                    r2 = img2.at<unsigned short>(img2.cols * j + i + 2);
                    break;
                case CV_32F: case CV_32S:
                    b2 = img2.at<float>(img2.cols * j + i) / 256;
                    g2 = img2.at<float>(img2.cols * j + i + 1) / 256;
                    r2 = img2.at<float>(img2.cols * j + i + 2) / 256;
                    break;
                case CV_64F:
                    b2 = img2.at<double>(img2.cols * j + i) / 65536;
                    g2 = img2.at<double>(img2.cols * j + i + 1) / 65536;
                    r2 = img2.at<double>(img2.cols * j + i + 2) / 65536;
                    break;
            }

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
