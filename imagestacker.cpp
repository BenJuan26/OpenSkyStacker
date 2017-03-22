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

void ImageStacker::process() {
    emit updateProgress("Starting stacking process...", 0);

    refImage = imread(refImageFileName.toUtf8().constData(), CV_LOAD_IMAGE_COLOR);

    switch (refImage.depth()) {
    case CV_8U: case CV_8S: default:
        refImage.convertTo(workingImage, CV_16UC3, 256);
        break;
    case CV_16U: case CV_16S:
        workingImage = refImage.clone();
        break;
    case CV_32F: case CV_32S:
        refImage.convertTo(workingImage, 1/256);
        break;
    case CV_64F:
        refImage.convertTo(workingImage, 1/65536);
        break;
    }

    QString message;

    for (int k = 0; k < targetImageFileNames.length() && !cancel; k++) {
        Mat targetImage = imread(targetImageFileNames.at(k).toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
        //targetImage.convertTo(targetImage, CV_32F);

        // -------------- ALIGNMENT ---------------
        message = "Aligning image " + QString::number(k+1) + " of " + QString::number(targetImageFileNames.length());
        qDebug() << message;
        emit updateProgress(message, k*2*100/(targetImageFileNames.length()*2));
        Mat targetAligned = generateAlignedImage(refImage, targetImage);


        if (cancel) break;


        // -------------- STACKING ---------------
        message = "Stacking image " + QString::number(k+1) + " of " + QString::number(targetImageFileNames.length());
        qDebug() << message;
        emit updateProgress(message, (k*2 + 1)*100/(targetImageFileNames.length()*2));
        workingImage = averageImages16UC3(workingImage, targetAligned);
    }

    if (cancel) {
        qDebug() << "Cancelled.";
    }
    else {
        emit finishedDialog("Stacking completed");
        emit finished(workingImage);
    }
}

cv::Mat ImageStacker::averageImages16UC3(cv::Mat img1, cv::Mat img2) {
    Mat result = Mat(img1.rows, img1.cols, CV_16UC3);

    for(int x = 0; x < img1.cols; x++) {
        for(int y = 0; y < img1.rows; y++) {

            unsigned short b1, g1, r1;
            switch (img1.depth()) {
                case CV_8U: case CV_8S: default: {
                    Vec<unsigned char, 3> pixel = img1.at<Vec<unsigned char,3>>(y,x);
                    b1 = pixel.val[0] * 256;
                    g1 = pixel.val[1] * 256;
                    r1 = pixel.val[2] * 256;
                    break;
                }
                case CV_16U: case CV_16S: {
                    Vec<unsigned short,3> pixel = img1.at<Vec<unsigned short,3>>(y,x);
                    b1 = pixel.val[0];
                    g1 = pixel.val[1];
                    r1 = pixel.val[2];
                    break;
                }
                case CV_32F: case CV_32S: {
                    Vec3f pixel = img1.at<Vec3f>(y,x);
                    b1 = pixel.val[0] / 256;
                    g1 = pixel.val[1] / 256;
                    r1 = pixel.val[2] / 256;
                    break;
                }
                case CV_64F: {
                    Vec3d pixel = img1.at<Vec3d>(y,x);
                    b1 = pixel.val[0] / 65536;
                    g1 = pixel.val[1] / 65536;
                    r1 = pixel.val[2] / 65536;
                    break;
                }
            }

            unsigned short b2, g2, r2;
            switch (img2.depth()) {
                case CV_8U: case CV_8S: default: {
                    Vec<unsigned char,3> pixel = img2.at<Vec<unsigned char,3>>(y,x);
                    b2 = pixel.val[0] * 256;
                    g2 = pixel.val[1] * 256;
                    r2 = pixel.val[2] * 256;
                    break;
                }
                case CV_16U: case CV_16S: {
                        Vec<unsigned short,3> pixel = img2.at<Vec<unsigned short,3>>(y,x);
                        b2 = pixel.val[0];
                        g2 = pixel.val[1];
                        r2 = pixel.val[2];
                        break;
                }
                case CV_32F: case CV_32S: {
                        Vec3f pixel = img2.at<Vec3f>(y,x);
                        b2 = pixel.val[0] / 256;
                        g2 = pixel.val[1] / 256;
                        r2 = pixel.val[2] / 256;
                        break;
                }
                case CV_64F: {
                        Vec3d pixel = img2.at<Vec3d>(y,x);
                        b2 = pixel.val[0] / 65536;
                        g2 = pixel.val[1] / 65536;
                        r2 = pixel.val[2] / 65536;
                        break;
                }
            }

            result.at<Vec<unsigned short,3>>(y,x).val[0] = (b1 + b2) / 2;
            result.at<Vec<unsigned short,3>>(y,x).val[1] = (g1 + g2) / 2;
            result.at<Vec<unsigned short,3>>(y,x).val[2] = (r1 + r2) / 2;
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

QString ImageStacker::getRefImageFileName() const {
    mutex.lock();
    QString string = refImageFileName;
    mutex.unlock();

    return string;
}
void ImageStacker::setRefImageFileName(const QString &value) {
    mutex.lock();
    refImageFileName = value;
    mutex.unlock();
}

QStringList ImageStacker::getTargetImageFileNames() const {
    mutex.lock();
    QStringList list = targetImageFileNames;
    mutex.unlock();

    return list;
}
void ImageStacker::setTargetImageFileNames(const QStringList &value) {
    mutex.lock();
    targetImageFileNames = value;
    mutex.unlock();
}

QStringList ImageStacker::getDarkFrameFileNames() const {
    mutex.lock();
    QStringList list = darkFrameFileNames;
    mutex.unlock();

    return list;
}
void ImageStacker::setDarkFrameFileNames(const QStringList &value) {
    mutex.lock();
    darkFrameFileNames = value;
    mutex.unlock();
}

QStringList ImageStacker::getDarkFlatFrameFileNames() const {
    mutex.lock();
    QStringList list = darkFlatFrameFileNames;
    mutex.unlock();

    return list;
}
void ImageStacker::setDarkFlatFrameFileNames(const QStringList &value) {
    mutex.lock();
    darkFlatFrameFileNames = value;
    mutex.unlock();
}

QStringList ImageStacker::getFlatFrameFileNames() const {
    mutex.lock();
    QStringList list = flatFrameFileNames;
    mutex.unlock();

    return list;
}
void ImageStacker::setFlatFrameFileNames(const QStringList &value) {
    mutex.lock();
    flatFrameFileNames = value;
    mutex.unlock();
}

QString ImageStacker::getSaveFilePath() const {
    mutex.lock();
    QString path = saveFilePath;
    mutex.unlock();

    return path;
}
void ImageStacker::setSaveFilePath(const QString &value) {
    mutex.lock();
    saveFilePath = value;
    mutex.unlock();
}

cv::Mat ImageStacker::getWorkingImage() const {
    mutex.lock();
    cv::Mat image = workingImage.clone();
    mutex.unlock();

    return image;
}
void ImageStacker::setWorkingImage(const cv::Mat &value) {
    mutex.lock();
    workingImage = value.clone();
    mutex.unlock();
}

cv::Mat ImageStacker::getRefImage() const {
    mutex.lock();
    cv::Mat image = refImage.clone();
    mutex.unlock();

    return image;
}
void ImageStacker::setRefImage(const cv::Mat &value) {
    mutex.lock();
    refImage = value.clone();
    mutex.unlock();
}

cv::Mat ImageStacker::getFinalImage() const {
    mutex.lock();
    cv::Mat image = finalImage.clone();
    mutex.unlock();

    return image;
}
void ImageStacker::setFinalImage(const cv::Mat &value) {
    mutex.lock();
    finalImage = value.clone();
    mutex.unlock();
}
