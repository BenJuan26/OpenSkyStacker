#include "imagestacker.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <QDebug>
#include <QTime>
#include <QFileInfo>

#ifdef WIN32
#define LIBRAW_NODLL
#endif
#include <libraw/libraw.h>

using namespace cv;

ImageStacker::ImageStacker(QObject *parent) : QObject(parent)
{
    cancel = false;
}

void ImageStacker::process() {
    emit updateProgress("Starting stacking process...", 0);

    refImage = readImage16UC3(refImageFileName);

    workingImage = refImage.clone();

    currentOperation = 0;

    totalOperations = targetImageFileNames.length() * 2 + 1;

    if (useDarks) {
        totalOperations += darkFrameFileNames.length();
    }
    if (useDarkFlats) {
        totalOperations += darkFlatFrameFileNames.length();
    }
    if (useFlats) {
        totalOperations += flatFrameFileNames.length();
    }

    if (useDarks) {
        stackDarks();
        workingImage -= masterDark;
    }
    if (useDarkFlats) {
        stackDarkFlats();
    }
    if (useFlats) {
        stackFlats();
        cv::divide(workingImage, masterFlat, workingImage, 1, CV_16U);
    }

    QString message;

    for (int k = 0; k < targetImageFileNames.length() && !cancel; k++) {
        Mat targetImage = readImage16UC3(targetImageFileNames.at(k));

        // ------------- CALIBRATION --------------
        if (useDarks) targetImage -= masterDark;
        if (useFlats)
            cv::divide(targetImage, masterFlat, targetImage, 1, CV_16U);

        // -------------- ALIGNMENT ---------------
        message = "Aligning image " + QString::number(k+1) + " of " + QString::number(targetImageFileNames.length());
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        Mat targetAligned = generateAlignedImage(refImage, targetImage);

        if (cancel) break;

        // -------------- STACKING ---------------
        message = "Stacking image " + QString::number(k+1) + " of " + QString::number(targetImageFileNames.length());
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

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

            Vec<unsigned short, 3> pixel1 = img1.at<Vec<unsigned short,3>>(y,x);
            unsigned short b1 = pixel1.val[0];
            unsigned short g1 = pixel1.val[1];
            unsigned short r1 = pixel1.val[2];

            Vec<unsigned short,3> pixel2 = img2.at<Vec<unsigned short,3>>(y,x);
            unsigned short b2 = pixel2.val[0];
            unsigned short g2 = pixel2.val[1];
            unsigned short r2 = pixel2.val[2];

            result.at<Vec<unsigned short,3>>(y,x).val[0] = (b1 + b2) / 2;
            result.at<Vec<unsigned short,3>>(y,x).val[1] = (g1 + g2) / 2;
            result.at<Vec<unsigned short,3>>(y,x).val[2] = (r1 + r2) / 2;
        }
    }

    return result;
}



void ImageStacker::stackDarks()
{
    Mat dark1 = readImage16UC3(darkFrameFileNames.at(0));
    Mat result = dark1.clone();

    QString message;

    for (int i = 0; i < darkFrameFileNames.length(); i++) {
        Mat dark = readImage16UC3(darkFrameFileNames.at(i));

        message = "Stacking dark frame " + QString::number(i+2) + " of " + QString::number(darkFrameFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);
        result = averageImages16UC3(result, dark);
    }

    masterDark = result;
}

void ImageStacker::stackDarkFlats()
{
    Mat darkFlat1 = readImage16UC3(darkFlatFrameFileNames.at(0));
    Mat result = darkFlat1.clone();

    QString message;

    for (int i = 0; i < darkFlatFrameFileNames.length(); i++) {
        Mat dark = readImage16UC3(darkFlatFrameFileNames.at(i));

        message = "Stacking dark flat frame " + QString::number(i+2) + " of " + QString::number(darkFlatFrameFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);
        result = averageImages16UC3(result, dark);
    }

    masterDarkFlat = result;
}

void ImageStacker::stackFlats()
{
    // most algorithms compute the median, but we will stick with mean for now
    Mat flat1 = readImage16UC3(flatFrameFileNames.at(0));
    Mat result = flat1.clone();

    if (useDarkFlats) result -= masterDarkFlat;

    QString message;

    for (int i = 0; i < flatFrameFileNames.length(); i++) {
        Mat flat = readImage16UC3(flatFrameFileNames.at(i));

        message = "Stacking flat frame " + QString::number(i+2) + " of " + QString::number(flatFrameFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);
        if (useDarkFlats) flat -= masterDarkFlat;
        result = averageImages16UC3(result, flat);
    }

    Scalar meanScalar = cv::mean(result);
    float avg = (meanScalar.val[0] + meanScalar.val[1] + meanScalar.val[2])/3;

    qDebug() << "Average: " << avg;
    result.convertTo(masterFlat, CV_32F, 1/avg);
}

Mat ImageStacker::convertAndScaleTo16UC3(Mat image)
{
    Mat result = Mat(image.rows, image.cols, CV_16UC3);

    switch (image.depth()) {
    case CV_8U: default:
        image.convertTo(result, CV_16U, 256);
        break;
    case CV_8S:
        image.convertTo(result, CV_16U, 256, 32768);
        break;
    case CV_16S:
        image.convertTo(result, CV_16U, 1, 32768);
        break;
    case CV_16U:
        // do nothing
        break;
    case CV_32S:
        image.convertTo(result, CV_16U, 1/256.0, 32768);
        break;
    case CV_32F: case CV_64F:
        image.convertTo(result, CV_16U, 65536);
        break;
    }

    return result;
}

Mat ImageStacker::rawTo16UC3(QString filename)
{
    LibRaw processor;

    // params for raw processing
    processor.imgdata.params.use_auto_wb = 0;
    processor.imgdata.params.use_camera_wb = 1;
    processor.imgdata.params.no_auto_bright = 1;
    processor.imgdata.params.output_bps = 16;
    processor.imgdata.params.no_auto_scale = 1;

    processor.open_file(filename.toUtf8().constData());
    processor.unpack();

    // process raw into readable BGR bitmap
    processor.dcraw_process();

    // TODO: ------ REVIEW MEMORY MANAGEMENT ------
    // I believe cv::Mat does not release the data automatically when it doesn't own the data
    //  (i.e. as in the constructor we're using here
    Mat image = Mat(Size(processor.imgdata.sizes.width, processor.imgdata.sizes.height),
                    CV_16UC3, processor.dcraw_make_mem_image());

    return image;
}

Mat ImageStacker::readImage16UC3(QString filename)
{
    Mat result;

    QFileInfo info(filename);
    QString ext = info.completeSuffix();

    if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
        if (totalOperations != 0) emit updateProgress("Reading raw file", 100*currentOperation/totalOperations);
        result = rawTo16UC3(filename);
    }
    else {
        result = imread(refImageFileName.toUtf8().constData(), CV_LOAD_IMAGE_COLOR);
        result = convertAndScaleTo16UC3(result);
    }

    return result;
}

cv::Mat ImageStacker::generateAlignedImage(Mat ref, Mat target) {
    // Convert images to gray scale;
    Mat ref_gray, target_gray;
    ref.convertTo(ref_gray, CV_8U, 1/256.0);
    target.convertTo(target_gray, CV_8U, 1/256.0);

    cvtColor(ref_gray, ref_gray, CV_BGR2GRAY);
    cvtColor(target_gray, target_gray, CV_BGR2GRAY);

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



// GETTER / SETTER

bool ImageStacker::getUseFlats() const
{
    mutex.lock();
    bool value = useFlats;
    mutex.unlock();

    return value;
}

void ImageStacker::setUseFlats(bool value)
{
    mutex.lock();
    useFlats = value;
    mutex.unlock();
}

bool ImageStacker::getUseDarkFlats() const
{
    mutex.lock();
    bool value = useDarkFlats;
    mutex.unlock();

    return value;
}

void ImageStacker::setUseDarkFlats(bool value)
{
    mutex.lock();
    useDarkFlats = value;
    mutex.unlock();
}

bool ImageStacker::getUseDarks() const
{
    mutex.lock();
    bool value = useDarks;
    mutex.unlock();

    return value;
}

void ImageStacker::setUseDarks(bool value)
{
    mutex.lock();
    useDarks = value;
    mutex.unlock();
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
