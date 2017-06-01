#include "imagestacker.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <QDebug>
#include <QTime>
#include <QFileInfo>
#include <ctime>

#ifdef WIN32
#define LIBRAW_NODLL
#endif
#include <libraw/libraw.h>

using namespace cv;

const std::vector<QString> ImageStacker::RAW_EXTENSIONS = {"3fr", "ari", "arw", "bay", "crw", "cr2",
        "cap", "data", "dcs", "dcr", "dng", "drf", "eip", "erf", "fff", "gpr", "iiq", "k25", "kdc",
        "mdc", "mef", "mos", "mrw", "nef", "nrw", "obm", "orf", "pef", "ptx", "pxn", "r3d", "raf",
        "raw", "rwl", "rw2", "rwz", "sr2", "srf", "srw", "x3f"};

ImageStacker::ImageStacker(QObject *parent) : QObject(parent)
{
    cancel = false;
    bitsPerChannel = BITS_32;
}

ImageRecord* ImageStacker::getImageRecord(QString filename)
{
    LibRaw processor;

    processor.open_file(filename.toUtf8().constData());

    libraw_imgother_t other = processor.imgdata.other;

    ImageRecord *record = new ImageRecord();
    record->setFilename(filename);
    record->setIso(other.iso_speed);
    record->setShutter(other.shutter);
    record->setTimestamp(other.timestamp);
    record->setWidth(processor.imgdata.sizes.width);
    record->setHeight(processor.imgdata.sizes.height);

    return record;
}

void ImageStacker::process() {
    cancel = false;
    emit updateProgress(tr("Checking image sizes"), 0);

    int err = validateImageSizes();
    if (err) {
        emit processingError("Images must all be the same size.");
        return;
    }

    currentOperation = 0;
    totalOperations = targetImageFileNames.length() * 2 + 1;

    if (useBias)      totalOperations += biasFrameFileNames.length();
    if (useDarks)     totalOperations += darkFrameFileNames.length();
    if (useDarkFlats) totalOperations += darkFlatFrameFileNames.length();
    if (useFlats)     totalOperations += flatFrameFileNames.length();

    if (useBias)      stackBias();
    if (useDarks)     stackDarks();
    if (useDarkFlats) stackDarkFlats();
    if (useFlats)     stackFlats();


    emit updateProgress(tr("Reading light frame 1 of %n", "", targetImageFileNames.length() + 1), 0);

    refImage = readImage(refImageFileName);

    if (useBias)  refImage -= masterBias;
    if (useDarks) refImage -= masterDark;
    if (useFlats) refImage /= masterFlat;

    // 32-bit float no matter what for the working image
    refImage.convertTo(workingImage, CV_32F);

    QString message;

    for (int k = 0; k < targetImageFileNames.length() && !cancel; k++) {
        // ---------------- LOAD -----------------
        message = tr("Reading light frame %1 of %2").arg(QString::number(k+2), QString::number(targetImageFileNames.length() + 1));
        qDebug() << message;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        Mat targetImage = readImage(targetImageFileNames.at(k));


        // ------------- CALIBRATION --------------
        message = tr("Calibrating light frame %1 of %2").arg(QString::number(k+2), QString::number(targetImageFileNames.length() + 1));
        qDebug() << message;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        if (useBias)  targetImage -= masterBias;
        if (useDarks) targetImage -= masterDark;
        if (useFlats) targetImage /= masterFlat;


        // -------------- ALIGNMENT ---------------
        message = tr("Aligning image %1 of %2").arg(QString::number(k+2), QString::number(targetImageFileNames.length() + 1));
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        Mat targetAligned = generateAlignedImage(refImage, targetImage);

        if (cancel) return;

        // -------------- STACKING ---------------
        message = tr("Stacking image %1 of %2").arg(QString::number(k+2), QString::number(targetImageFileNames.length() + 1));
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        cv::add(workingImage, targetAligned, workingImage, noArray(), CV_32F);
    }

    if (cancel) return;

    workingImage /= targetImageFileNames.length() + 1;

    // only need to change the bit depth, no scaling
    if (bitsPerChannel == BITS_16) {
        workingImage.convertTo(workingImage, CV_16U);
    }

    emit finishedDialog(tr("Stacking completed"));
    emit finished(workingImage);
}

void ImageStacker::readQImage(QString filename)
{
    cv::Mat image = readImage(filename);

    double min, max;
    cv::minMaxLoc(image, &min, &max);

    // stretch intensity levels
    image *= (1.0/max);

    emit QImageReady(Mat2QImage(image));
}

cv::Mat ImageStacker::averageImages(cv::Mat img1, cv::Mat img2) {
    Mat result;

    switch (bitsPerChannel) {
    case BITS_16: {
        result = Mat(img1.rows, img1.cols, CV_16UC3);
        for(int x = 0; x < img1.cols; x++) {
            for(int y = 0; y < img1.rows; y++) {

                Vec<unsigned short, 3> pixel1 = img1.at< Vec<unsigned short,3> >(y,x);
                unsigned short b1 = pixel1.val[0];
                unsigned short g1 = pixel1.val[1];
                unsigned short r1 = pixel1.val[2];

                Vec<unsigned short,3> pixel2 = img2.at< Vec<unsigned short,3> >(y,x);
                unsigned short b2 = pixel2.val[0];
                unsigned short g2 = pixel2.val[1];
                unsigned short r2 = pixel2.val[2];

                result.at< Vec<unsigned short,3> >(y,x).val[0] = (b1 + b2) / 2;
                result.at< Vec<unsigned short,3> >(y,x).val[1] = (g1 + g2) / 2;
                result.at< Vec<unsigned short,3> >(y,x).val[2] = (r1 + r2) / 2;
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

int ImageStacker::validateImageSizes()
{
    Mat ref = readImage(refImageFileName);

    int width = ref.cols;
    int height = ref.rows;

    qDebug() << "refImage:" << width << height;

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        QString filename = targetImageFileNames.at(i);

        Mat image = readImage(filename);

        if (image.cols != width ||  image.rows != height) {
            return -1;
        }
    }

    if (useBias) {
        for (int i = 0; i < biasFrameFileNames.length(); i++) {
            QString filename = biasFrameFileNames.at(i);

            Mat image = readImage(filename);

            if (image.cols != width ||  image.rows != height) {
                return -1;
            }
        }
    }

    if (useDarks) {
        for (int i = 0; i < darkFrameFileNames.length(); i++) {
            QString filename = darkFrameFileNames.at(i);

            Mat image = readImage(filename);

            if (image.cols != width ||  image.rows != height) {
                return -1;
            }
        }
    }

    if (useDarkFlats) {
        for (int i = 0; i < darkFlatFrameFileNames.length(); i++) {
            QString filename = darkFlatFrameFileNames.at(i);

            Mat image = readImage(filename);

            if (image.cols != width ||  image.rows != height) {
                return -1;
            }
        }
    }

    if (useFlats) {
        for (int i = 0; i < flatFrameFileNames.length(); i++) {
            QString filename = flatFrameFileNames.at(i);

            Mat image = readImage(filename);

            if (image.cols != width ||  image.rows != height) {
                return -1;
            }
        }
    }

    return 0;
}

QImage ImageStacker::Mat2QImage(const Mat &src)
{
    QImage dest(src.cols, src.rows, QImage::Format_RGB32);
    int r, g, b;

    if (getBitsPerChannel() == ImageStacker::BITS_16) {
        for(int x = 0; x < src.cols; x++) {
            for(int y = 0; y < src.rows; y++) {

                Vec<unsigned short,3> pixel = src.at< Vec<unsigned short,3> >(y,x);
                b = pixel.val[0]/256;
                g = pixel.val[1]/256;
                r = pixel.val[2]/256;
                dest.setPixel(x, y, qRgb(r,g,b));
            }
        }
    }
    else if (getBitsPerChannel() == ImageStacker::BITS_32) {
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

void ImageStacker::stackDarks()
{
    Mat dark1 = readImage(darkFrameFileNames.at(0));
    if (useBias) dark1 -= masterBias;

    Mat result = dark1.clone();

    QString message;

    for (int i = 1; i < darkFrameFileNames.length() && !cancel; i++) {
        Mat dark = readImage(darkFrameFileNames.at(i));

        message = tr("Stacking dark frame %1 of %2").arg(QString::number(i+1), QString::number(darkFrameFileNames.length()));
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        if (useBias) dark -= masterBias;
        result += dark;
    }

    result /= darkFrameFileNames.length();

    masterDark = result;
}

void ImageStacker::stackDarkFlats()
{
    Mat darkFlat1 = readImage(darkFlatFrameFileNames.at(0));
    if (useBias) darkFlat1 -= masterBias;

    Mat result = darkFlat1.clone();

    QString message;

    for (int i = 1; i < darkFlatFrameFileNames.length() && !cancel; i++) {
        Mat dark = readImage(darkFlatFrameFileNames.at(i));

        message = tr("Stacking dark flat frame %1 of %2").arg(QString::number(i+1), QString::number(darkFlatFrameFileNames.length()));
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        if (useBias) dark -= masterBias;
        result += dark;
    }

    result /= darkFlatFrameFileNames.length();

    masterDarkFlat = result;
}

void ImageStacker::stackFlats()
{
    // most algorithms compute the median, but we will stick with mean for now
    Mat flat1 = readImage(flatFrameFileNames.at(0));
    Mat result = flat1.clone();

    if (useBias) result -= masterBias;
    if (useDarkFlats) result -= masterDarkFlat;

    QString message;

    for (int i = 1; i < flatFrameFileNames.length() && !cancel; i++) {
        Mat flat = readImage(flatFrameFileNames.at(i));

        message = tr("Stacking flat frame %1 of %2").arg(QString::number(i+1), QString::number(flatFrameFileNames.length()));
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        if (useBias) flat -= masterBias;
        if (useDarkFlats) flat -= masterDarkFlat;
        result += flat;
    }

    result /= flatFrameFileNames.length();

    // scale the flat frame so that the average value is 1.0
    // since we're dividing, flat values darker than the average value will brighten the image
    //  and values brighter than the average will darken the image, flattening the field
    Scalar meanScalar = cv::mean(result);
    float avg = (meanScalar.val[0] + meanScalar.val[1] + meanScalar.val[2])/3;

    qDebug() << "Average: " << avg;
    if (bitsPerChannel == BITS_16)
        result.convertTo(masterFlat, CV_32F, 1.0/avg);
    else if (bitsPerChannel == BITS_32)
        masterFlat = result / avg;
}

void ImageStacker::stackBias()
{
    Mat bias1 = readImage(biasFrameFileNames.at(0));
    Mat result = bias1.clone();

    QString message;

    for (int i = 1; i < biasFrameFileNames.length() && !cancel; i++) {
        Mat bias = readImage(biasFrameFileNames.at(i));

        message = tr("Stacking bias frame %1 of %2").arg(QString::number(i+1), QString::number(biasFrameFileNames.length()));
        qDebug() << message;
        currentOperation++;
        if (totalOperations != 0) emit updateProgress(message, 100*currentOperation/totalOperations);

        result += bias;
    }

    result /= biasFrameFileNames.length();

    masterBias = result;
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

// derived from FOCAS mktransform.c
cv::Mat ImageStacker::generateAlignedImage(Mat ref, Mat target) {
    StarDetector sd;
    std::vector<Star> List1 = sd.getStars(ref);
    std::vector<Star> List2 = sd.getStars(target);

    std::vector<Triangle> List_triangA = generateTriangleList(List1);
    std::vector<Triangle> List_triangB = generateTriangleList(List2);

    int nobjs = 40;

    int k = 0;
    std::vector< std::vector<int> > matches = findMatches(nobjs, &k, List_triangA, List_triangB);
    std::vector< std::vector<float> > transformVec = findTransform(matches, k, List1, List2);

    cv::Mat matTransform(2,3,CV_32F);
    for(int i = 0; i < 2; i++)
        for (int j = 0; j < 3; j++)
            matTransform.at<float>(i,j) = transformVec[i][j];

    warpAffine(target, target, matTransform, target.size(), INTER_LINEAR + WARP_INVERSE_MAP);

    return target;
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

bool ImageStacker::getUseBias() const
{
    mutex.lock();
    bool value = useBias;
    mutex.unlock();

    return value;
}

void ImageStacker::setUseBias(bool value)
{
    mutex.lock();
    useBias = value;
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

QStringList ImageStacker::getBiasFrameFileNames() const
{
    mutex.lock();
    QStringList list = biasFrameFileNames;
    mutex.unlock();

    return list;
}

void ImageStacker::setBiasFrameFileNames(const QStringList &value)
{
    mutex.lock();
    biasFrameFileNames = value;
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
