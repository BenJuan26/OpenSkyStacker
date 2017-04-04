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

const std::vector<QString> ImageStacker::RAW_EXTENSIONS = {"3fr", "ari", "arw", "bay", "crw", "cr2",
                                                                  "cap", "data", "dcs", "dcr", "dng", "drf", "eip", "erf", "fff", "gpr",
                                                                  "iiq", "k25", "kdc", "mdc", "mef", "mos", "mrw", "nef", "nrw", "obm",
                                                                  "orf", "pef", "ptx", "pxn", "r3d", "raf", "raw", "rwl", "rw2", "rwz",
                                                                  "sr2", "srf", "srw", "x3f"};

ImageStacker::ImageStacker(QObject *parent) : QObject(parent)
{
    cancel = false;
    bitsPerChannel = BITS_16;
}

void ImageStacker::process() {
    emit updateProgress("Starting stacking process...", 0);

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
    }
    if (useDarkFlats) {
        stackDarkFlats();
    }
    if (useFlats) {
        stackFlats();
    }

    emit updateProgress("Reading light frame 1 of " + QString::number(targetImageFileNames.length() + 1), 0);

    refImage = readImage(refImageFileName);
    if (useDarks) {
        refImage -= masterDark;
    }
    if (useFlats) {
        if (bitsPerChannel == BITS_16)
            cv::divide(refImage, masterFlat, refImage, 1, CV_16U);
        else if (bitsPerChannel == BITS_32)
            cv::divide(refImage, masterFlat, refImage, 1, CV_32F);
    }

    // 32-bit float no matter what for the working image
    refImage.convertTo(workingImage, CV_32F);

    QString message;

    for (int k = 0; k < targetImageFileNames.length() && !cancel; k++) {
        message = "Reading light frame " + QString::number(k+2) + " of " + QString::number(targetImageFileNames.length()+1);
        qDebug() << message;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);
        Mat targetImage = readImage(targetImageFileNames.at(k));


        message = "Calibrating light frame " + QString::number(k+2) + " of " + QString::number(targetImageFileNames.length()+1);
        qDebug() << message;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        // ------------- CALIBRATION --------------
        if (useDarks) targetImage -= masterDark;
        if (useFlats) {
            if (bitsPerChannel == BITS_16)
                cv::divide(targetImage, masterFlat, targetImage, 1, CV_16U);
            else if (bitsPerChannel == BITS_32)
                cv::divide(targetImage, masterFlat, targetImage, 1, CV_32F);
        }

        // -------------- ALIGNMENT ---------------
        message = "Aligning image " + QString::number(k+2) + " of " + QString::number(targetImageFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        Mat targetAligned = generateAlignedImage(refImage, targetImage);

        if (cancel) break;

        // -------------- STACKING ---------------
        message = "Stacking image " + QString::number(k+2) + " of " + QString::number(targetImageFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        cv::add(workingImage, targetAligned, workingImage, noArray(), CV_32F);
    }

    workingImage /= targetImageFileNames.length() + 1;

    // only need to change the bit depth, no scaling
    if (bitsPerChannel == BITS_16) {
        workingImage.convertTo(workingImage, CV_16U);
    }

    if (cancel) {
        qDebug() << "Cancelled.";
    }
    else {
        emit finishedDialog("Stacking completed");
        emit finished(workingImage);
    }
}

cv::Mat ImageStacker::averageImages(cv::Mat img1, cv::Mat img2) {
    Mat result;

    switch (bitsPerChannel) {
    case BITS_16: {
        result = Mat(img1.rows, img1.cols, CV_16UC3);
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

        break;
    }

    case BITS_32: {
        result = Mat(img1.rows, img1.cols, CV_32FC3);
        for(int x = 0; x < img1.cols; x++) {
            for(int y = 0; y < img1.rows; y++) {

                Vec3f pixel1 = img1.at<Vec3f>(y,x);
                float b1 = pixel1.val[0];
                float g1 = pixel1.val[1];
                float r1 = pixel1.val[2];

                Vec3f pixel2 = img2.at<Vec3f>(y,x);
                float b2 = pixel2.val[0];
                float g2 = pixel2.val[1];
                float r2 = pixel2.val[2];

                result.at<Vec3f>(y,x).val[0] = (b1 + b2) / 2.0;
                result.at<Vec3f>(y,x).val[1] = (g1 + g2) / 2.0;
                result.at<Vec3f>(y,x).val[2] = (r1 + r2) / 2.0;
            }
        }

        break;
    }
    }

    return result;
}

void ImageStacker::stackDarks()
{
    Mat dark1 = readImage(darkFrameFileNames.at(0));
    Mat result = dark1.clone();

    QString message;

    for (int i = 0; i < darkFrameFileNames.length() && !cancel; i++) {
        Mat dark = readImage(darkFrameFileNames.at(i));

        message = "Stacking dark frame " + QString::number(i+2) + " of " + QString::number(darkFrameFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);
        result = averageImages(result, dark);
    }

    masterDark = result;
}

void ImageStacker::stackDarkFlats()
{
    Mat darkFlat1 = readImage(darkFlatFrameFileNames.at(0));
    Mat result = darkFlat1.clone();

    QString message;

    for (int i = 0; i < darkFlatFrameFileNames.length() && !cancel; i++) {
        Mat dark = readImage(darkFlatFrameFileNames.at(i));

        message = "Stacking dark flat frame " + QString::number(i+2) + " of " + QString::number(darkFlatFrameFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);
        result = averageImages(result, dark);
    }

    masterDarkFlat = result;
}

void ImageStacker::stackFlats()
{
    // most algorithms compute the median, but we will stick with mean for now
    Mat flat1 = readImage(flatFrameFileNames.at(0));
    Mat result = flat1.clone();

    if (useDarkFlats) result -= masterDarkFlat;

    QString message;

    for (int i = 0; i < flatFrameFileNames.length() && !cancel; i++) {
        Mat flat = readImage(flatFrameFileNames.at(i));

        message = "Stacking flat frame " + QString::number(i+2) + " of " + QString::number(flatFrameFileNames.length()+1);
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);
        if (useDarkFlats) flat -= masterDarkFlat;
        result = averageImages(result, flat);
    }

    Scalar meanScalar = cv::mean(result);
    float avg = (meanScalar.val[0] + meanScalar.val[1] + meanScalar.val[2])/3;

    qDebug() << "Average: " << avg;
    if (bitsPerChannel == BITS_16)
        result.convertTo(masterFlat, CV_32F, 1/avg);
    else if (bitsPerChannel == BITS_32)
        masterFlat = result / avg;
}

Mat ImageStacker::convertAndScaleImage(Mat image)
{
    Mat result;

    if (bitsPerChannel == BITS_16) {
        result = Mat(image.rows, image.cols, CV_16UC3);
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
            result = image.clone();
            break;
        case CV_32S:
            image.convertTo(result, CV_16U, 1/256.0, 32768);
            break;
        case CV_32F: case CV_64F:
            image.convertTo(result, CV_16U, 65536);
            break;
        }
    }
    else if (bitsPerChannel == BITS_32) {
        result = Mat(image.rows, image.cols, CV_32FC3);
        switch (image.depth()) {
        case CV_8U: default:
            image.convertTo(result, CV_32F, 1/255.0);
            break;
        case CV_8S:
            image.convertTo(result, CV_32F, 1/255.0, 1.0);
            break;
        case CV_16S:
            image.convertTo(result, CV_32F, 1/65535.0, 1.0);
            break;
        case CV_16U:
            image.convertTo(result, CV_32F, 1/65535.0);
            break;
        case CV_32S:
            image.convertTo(result, CV_32F, 1.0/(2^31 - 1), 1.0);
            break;
        case CV_32F:
            result = image.clone();
            break;
        case CV_64F:
            image.convertTo(result, CV_32F);
            break;
        }
    }
    return result;
}

Mat ImageStacker::rawToMat(QString filename)
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

    libraw_processed_image_t *proc = processor.dcraw_make_mem_image();
    Mat tmp = Mat(Size(processor.imgdata.sizes.width, processor.imgdata.sizes.height),
                    CV_16UC3, proc->data);

    // copy data -- slower, but then we can rely on OpenCV's reference counting
    // also, we have to convert RGB->BGR anyway
    Mat image = Mat(processor.imgdata.sizes.width, processor.imgdata.sizes.height, CV_16UC3);
    cvtColor(tmp, image, CV_RGB2BGR);

    if (bitsPerChannel == BITS_32)
        image.convertTo(image, CV_32F, 1/65535.0);

    // free the memory that otherwise wouldn't have been handled by OpenCV
    delete proc;
    processor.recycle();

    return image;
}

Mat ImageStacker::readImage(QString filename)
{
    Mat result;

    QFileInfo info(filename);
    QString ext = info.completeSuffix();

    // assumption: if it looks like a raw file, it is a raw file
    if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
        result = rawToMat(filename);
    }
    else {
        result = imread(filename.toUtf8().constData(), CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        result = convertAndScaleImage(result);
    }

    return result;
}

cv::Mat ImageStacker::generateAlignedImage(Mat ref, Mat target) {
    StarDetector sd;
    std::vector<Star> List1 = sd.getStars(ref);
    std::vector<Star> List2 = sd.getStars(target);

    std::vector<Triangle> List_triangA = generateTriangleList(List1);
    std::vector<Triangle> List_triangB = generateTriangleList(List2);

    int nobjs = 40;

    int k;
    std::vector<std::vector<int> > matches = findMatches(nobjs, &k, List_triangA, List_triangB, List1, List2);
    std::vector<std::vector<float> > transformVec = findTransform(matches, k, List1, List2);

    cv::Mat matTransform(2,3,CV_32F);
    for(int i = 0; i < 2; i++)
        for (int j = 0; j < 3; j++)
            matTransform.at<float>(i,j) = transformVec[i][j];

    warpAffine(target, target, matTransform, target.size(), INTER_LINEAR + WARP_INVERSE_MAP);
    imwrite("F:\\Astro\\Samples\\Img1781.tif", target);

    return target;
}

cv::Mat ImageStacker::generateAlignedImageOld(Mat ref, Mat target) {
    // Convert images to gray scale;
    Mat ref_gray, target_gray;
    float scale = 256;
    if (bitsPerChannel == BITS_16) scale = 1/256.0;
    ref.convertTo(ref_gray, CV_8U, scale);
    target.convertTo(target_gray, CV_8U, scale);

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

ImageStacker::BITS_PER_CHANNEL ImageStacker::getBitsPerChannel() const
{
    mutex.lock();
    BITS_PER_CHANNEL value = bitsPerChannel;
    mutex.unlock();

    return value;
}

void ImageStacker::setBitsPerChannel(const BITS_PER_CHANNEL &value)
{
    mutex.lock();
    bitsPerChannel = value;
    mutex.unlock();
}

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
