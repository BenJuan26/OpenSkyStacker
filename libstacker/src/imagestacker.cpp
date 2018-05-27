#include "libstacker/imagestacker.h"
#include "imagestacker_p.h"

#include "focas.h"
#include "exif.h"

#include <QMutex>
#include <QImage>
#include <QDebug>
#include <QTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include <ctime>

#ifdef WIN32
#define LIBRAW_NODLL
#endif
#include <libraw.h>

#include <CCfits/CCfits>

using namespace openskystacker;
using namespace CCfits;
using namespace easyexif;

ImageStacker::ImageStacker(QObject *parent) : QObject(parent),
        d_ptr(new ImageStackerPrivate(this)),
        cancel(false)
{

}

ImageStacker::~ImageStacker() {}

void ImageStacker::process(int tolerance, int threads) {
    Q_D(ImageStacker);
    d->process(tolerance, threads);
}

void ImageStacker::readQImage(QString filename)
{
    Q_D(ImageStacker);
    d->readQImage(filename);
}

void ImageStacker::detectStars(QString filename, int threshold)
{
    Q_D(ImageStacker);
    d->detectStars(filename, threshold);
}

// GETTER / SETTER

bool ImageStacker::getUseFlats() const
{
    Q_D(const ImageStacker);
    return d->getUseFlats();
}

void ImageStacker::setUseFlats(bool value)
{
    Q_D(ImageStacker);
    d->setUseFlats(value);
}

bool ImageStacker::getUseDarkFlats() const
{
    Q_D(const ImageStacker);
    return d->getUseDarkFlats();
}

void ImageStacker::setUseDarkFlats(bool value)
{
    Q_D(ImageStacker);
    d->setUseDarkFlats(value);
}

bool ImageStacker::getUseDarks() const
{
    Q_D(const ImageStacker);
    return d->getUseDarks();
}

void ImageStacker::setUseDarks(bool value)
{
    Q_D(ImageStacker);
    d->setUseDarks(value);
}

bool ImageStacker::getUseBias() const
{
    Q_D(const ImageStacker);
    return d->getUseBias();
}

void ImageStacker::setUseBias(bool value)
{
    Q_D(ImageStacker);
    d->setUseBias(value);
}

QString ImageStacker::getRefImageFileName() const {
    Q_D(const ImageStacker);
    return d->getRefImageFileName();
}
void ImageStacker::setRefImageFileName(const QString &value) {
    Q_D(ImageStacker);
    d->setRefImageFileName(value);
}

QStringList ImageStacker::getTargetImageFileNames() const {
    Q_D(const ImageStacker);
    return d->getTargetImageFileNames();
}
void ImageStacker::setTargetImageFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->setTargetImageFileNames(value);
}

QStringList ImageStacker::getDarkFrameFileNames() const {
    Q_D(const ImageStacker);
    return d->getDarkFrameFileNames();
}
void ImageStacker::setDarkFrameFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->setDarkFrameFileNames(value);
}

QStringList ImageStacker::getDarkFlatFrameFileNames() const {
    Q_D(const ImageStacker);
    return d->getDarkFlatFrameFileNames();
}
void ImageStacker::setDarkFlatFrameFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->setDarkFlatFrameFileNames(value);
}

QStringList ImageStacker::getFlatFrameFileNames() const {
    Q_D(const ImageStacker);
    return d->getFlatFrameFileNames();
}
void ImageStacker::setFlatFrameFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->setFlatFrameFileNames(value);
}

QStringList ImageStacker::getBiasFrameFileNames() const
{
    Q_D(const ImageStacker);
    return d->getBiasFrameFileNames();
}

void ImageStacker::setBiasFrameFileNames(const QStringList &value)
{
    Q_D(ImageStacker);
    d->setBiasFrameFileNames(value);
}

QString ImageStacker::getSaveFilePath() const {
    Q_D(const ImageStacker);
    return d->getSaveFilePath();
}
void ImageStacker::setSaveFilePath(const QString &value) {
    Q_D(ImageStacker);
    d->setSaveFilePath(value);
}

cv::Mat ImageStacker::getWorkingImage() const {
    Q_D(const ImageStacker);
    return d->getWorkingImage();
}
void ImageStacker::setWorkingImage(const cv::Mat &value) {
    Q_D(ImageStacker);
    d->setWorkingImage(value);
}

cv::Mat ImageStacker::getRefImage() const {
    Q_D(const ImageStacker);
    return d->getRefImage();
}
void ImageStacker::setRefImage(const cv::Mat &value) {
    Q_D(ImageStacker);
    d->setRefImage(value);
}

cv::Mat ImageStacker::getFinalImage() const {
    Q_D(const ImageStacker);
    return d->getFinalImage();
}
void ImageStacker::setFinalImage(const cv::Mat &value) {
    Q_D(ImageStacker);
    d->setFinalImage(value);
}








ImageStackerPrivate::ImageStackerPrivate(ImageStacker *parent) : q_ptr(parent)
{

}

int ImageStackerPrivate::getTotalOperations()
{
    int ops = targetImageFileNames.length() + 4;

    if (useBias)       ops += 1;
    if (useDarks)      ops += 1;
    if (useDarkFlats) ops += 1;
    if (useFlats)      ops += 1;

    return ops;
}

void ImageStackerPrivate::process(int tolerance, int threads) {
    Q_Q(ImageStacker);
    time_t now;
    time(&now);

    ImageType refType = getImageType(refImageFileName);

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        if (getImageType(targetImageFileNames.at(i)) != refType) {
            emit q->processingError(QObject::tr("Images must be the same type."));
            return;
        }
    }

    q->cancel = false;
    currentOperation = 1;
    totalOperations = getTotalOperations();
    emit q->updateProgress(QObject::tr("Checking image sizes..."), 100 * currentOperation++ / totalOperations);

    int err = validateImageSizes();
    if (err) {
        emit q->processingError("Images must all be the same size.");
        return;
    }

    cv::Mat masterDark, masterDarkFlat, masterFlat, masterBias;

    if (useBias) {
        emit q->updateProgress(QObject::tr("Stacking bias frames..."), 100 * currentOperation++ / totalOperations);
        masterBias = stackBias(biasFrameFileNames);
    }
    if (useDarks) {
        emit q->updateProgress(QObject::tr("Stacking dark frames..."), 100 * currentOperation++ / totalOperations);
        masterDark = stackDarks(darkFrameFileNames, masterBias);
    }
    if (useDarkFlats) {
        emit q->updateProgress(QObject::tr("Stacking dark flat frames..."), 100 * currentOperation++ / totalOperations);
        masterDarkFlat = stackDarkFlats(darkFlatFrameFileNames, masterBias);
    }
    if (useFlats) {
        emit q->updateProgress(QObject::tr("Stacking flat frames..."), 100 * currentOperation++ / totalOperations);
        masterFlat = stackFlats(flatFrameFileNames, masterDarkFlat, masterBias);
    }

    emit q->updateProgress(QObject::tr("Stacking light frames..."), 100 * currentOperation++ / totalOperations);

    refImage = getCalibratedImage(refImageFileName, masterDark , masterFlat, masterBias);
    workingImage = refImage.clone();

    int totalValidImages = 1;
    if (threads >= targetImageFileNames.length())
        threads = targetImageFileNames.length() - 1;

    StackingParams params;
    params.lights = targetImageFileNames;
    params.ref = refImage;
    params.masterDark = masterDark;
    params.masterFlat = masterFlat;
    params.masterBias = masterBias;
    params.tolerance = tolerance;
    params.totalThreads = threads;

    std::vector< QFuture<StackingResult> > futures;
    std::vector<int *> completes;
    for (int i = 0; i < threads; i++) {
        int *c = new int;
        *c = 0;
        completes.push_back(c);
        params.threadIndex = i;
        QFuture<StackingResult> future = QtConcurrent::run(processConcurrent, params, c);
        futures.push_back(future);
    }

    bool done = false;
    while (!done) {
        done = true;
        int op = currentOperation;
        int i = 0;
        for (auto &future : futures) {
            op += *completes.at(i);
            done = done && future.isFinished();
            i++;
        }

        emit q->updateProgress(QObject::tr("Stacking light frames..."), 100 * op / totalOperations);
        QThread::msleep(250);
    }

    for (QFuture<StackingResult> future : futures) {
        workingImage += future.result().image;
        totalValidImages += future.result().totalValidImages;
    }

    if (totalValidImages < 2) {
        emit q->processingError(QObject::tr("No images could be aligned to the reference image. Try using a lower tolerance."));
        return;
    }

    workingImage /= totalValidImages;

    // LibRaw works in RGB while OpenCV works in BGR
    if (getImageType(refImageFileName) == RAW_IMAGE)
        cv::cvtColor(workingImage, workingImage, CV_RGB2BGR);

    time_t doneStacking;
    time(&doneStacking);

    QString completeString = QObject::tr("Stacking completed in ");
    QString timeString = getTimeString(difftime(doneStacking, now));

    emit q->finished(workingImage, completeString + timeString + ".");
}

void ImageStackerPrivate::readQImage(QString filename)
{
    Q_Q(ImageStacker);
    cv::Mat image = readImage(filename);

    double min, max;
    cv::minMaxLoc(image, &min, &max);

    // stretch intensity levels
    image *= (1.0/max);

    emit q->qImageReady(mat2QImage(image));
}

void ImageStackerPrivate::detectStars(QString filename, int threshold)
{
    Q_Q(ImageStacker);
    cv::Mat image = readImage(filename);
    StarDetector sd;
    std::vector<Star> list = sd.getStars(image, threshold);

    emit q->doneDetectingStars(static_cast<int>(list.size()));
}

int ImageStackerPrivate::validateImageSizes()
{
    LibRaw processor;

    // params for raw processing
    processor.imgdata.params.use_auto_wb = 0;
    processor.imgdata.params.use_camera_wb = 1;
    processor.imgdata.params.no_auto_bright = 1;
    processor.imgdata.params.output_bps = 16;

    QFileInfo info(refImageFileName);
    QString ext = info.completeSuffix();
    int refWidth;
    int refHeight;

    if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
        processor.open_file(refImageFileName.toUtf8().constData());
        refWidth = processor.imgdata.sizes.width;
        refHeight = processor.imgdata.sizes.height;
        processor.free_image();
    } else {
        cv::Mat ref = readImage(refImageFileName);
        refWidth = ref.cols;
        refHeight = ref.rows;
    }

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        QString filename = targetImageFileNames.at(i);

        int width;
        int height;

        if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
            processor.open_file(filename.toUtf8().constData());
            width = processor.imgdata.sizes.width;
            height = processor.imgdata.sizes.height;
            processor.free_image();
        } else {
            cv::Mat ref = readImage(filename);
            width = ref.cols;
            height = ref.rows;
        }

        if (width != refWidth ||  height != refHeight) {
            return -1;
        }
    }

    if (useBias) {
        for (int i = 0; i < biasFrameFileNames.length(); i++) {
            QString filename = biasFrameFileNames.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = readImage(filename);
                width = ref.cols;
                height = ref.rows;
            }

            if (width != refWidth ||  height != refHeight) {
                return -1;
            }
        }
    }

    if (useDarks) {
        for (int i = 0; i < darkFrameFileNames.length(); i++) {
            QString filename = darkFrameFileNames.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = readImage(filename);
                width = ref.cols;
                height = ref.rows;
            }

            if (width != refWidth ||  height != refHeight) {
                return -1;
            }
        }
    }

    if (useDarkFlats) {
        for (int i = 0; i < darkFlatFrameFileNames.length(); i++) {
            QString filename = darkFlatFrameFileNames.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = readImage(filename);
                width = ref.cols;
                height = ref.rows;
            }

            if (width != refWidth ||  height != refHeight) {
                return -1;
            }
        }
    }

    if (useFlats) {
        for (int i = 0; i < flatFrameFileNames.length(); i++) {
            QString filename = flatFrameFileNames.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = readImage(filename);
                width = ref.cols;
                height = ref.rows;
            }

            if (width != refWidth ||  height != refHeight) {
                return -1;
            }
        }
    }

    return 0;
}































// GETTER / SETTER

bool ImageStackerPrivate::getUseFlats() const
{
    mutex.lock();
    bool value = useFlats;
    mutex.unlock();

    return value;
}

void ImageStackerPrivate::setUseFlats(bool value)
{
    mutex.lock();
    useFlats = value;
    mutex.unlock();
}

bool ImageStackerPrivate::getUseDarkFlats() const
{
    mutex.lock();
    bool value = useDarkFlats;
    mutex.unlock();

    return value;
}

void ImageStackerPrivate::setUseDarkFlats(bool value)
{
    mutex.lock();
    useDarkFlats = value;
    mutex.unlock();
}

bool ImageStackerPrivate::getUseDarks() const
{
    mutex.lock();
    bool value = useDarks;
    mutex.unlock();

    return value;
}

void ImageStackerPrivate::setUseDarks(bool value)
{
    mutex.lock();
    useDarks = value;
    mutex.unlock();
}

bool ImageStackerPrivate::getUseBias() const
{
    mutex.lock();
    bool value = useBias;
    mutex.unlock();

    return value;
}

void ImageStackerPrivate::setUseBias(bool value)
{
    mutex.lock();
    useBias = value;
    mutex.unlock();
}

QString ImageStackerPrivate::getRefImageFileName() const {
    mutex.lock();
    QString string = refImageFileName;
    mutex.unlock();

    return string;
}
void ImageStackerPrivate::setRefImageFileName(const QString &value) {
    mutex.lock();
    refImageFileName = value;
    mutex.unlock();
}

QStringList ImageStackerPrivate::getTargetImageFileNames() const {
    mutex.lock();
    QStringList list = targetImageFileNames;
    mutex.unlock();

    return list;
}
void ImageStackerPrivate::setTargetImageFileNames(const QStringList &value) {
    mutex.lock();
    targetImageFileNames = value;
    mutex.unlock();
}

QStringList ImageStackerPrivate::getDarkFrameFileNames() const {
    mutex.lock();
    QStringList list = darkFrameFileNames;
    mutex.unlock();

    return list;
}
void ImageStackerPrivate::setDarkFrameFileNames(const QStringList &value) {
    mutex.lock();
    darkFrameFileNames = value;
    mutex.unlock();
}

QStringList ImageStackerPrivate::getDarkFlatFrameFileNames() const {
    mutex.lock();
    QStringList list = darkFlatFrameFileNames;
    mutex.unlock();

    return list;
}
void ImageStackerPrivate::setDarkFlatFrameFileNames(const QStringList &value) {
    mutex.lock();
    darkFlatFrameFileNames = value;
    mutex.unlock();
}

QStringList ImageStackerPrivate::getFlatFrameFileNames() const {
    mutex.lock();
    QStringList list = flatFrameFileNames;
    mutex.unlock();

    return list;
}
void ImageStackerPrivate::setFlatFrameFileNames(const QStringList &value) {
    mutex.lock();
    flatFrameFileNames = value;
    mutex.unlock();
}

QStringList ImageStackerPrivate::getBiasFrameFileNames() const
{
    mutex.lock();
    QStringList list = biasFrameFileNames;
    mutex.unlock();

    return list;
}

void ImageStackerPrivate::setBiasFrameFileNames(const QStringList &value)
{
    mutex.lock();
    biasFrameFileNames = value;
    mutex.unlock();
}

QString ImageStackerPrivate::getSaveFilePath() const {
    mutex.lock();
    QString path = saveFilePath;
    mutex.unlock();

    return path;
}
void ImageStackerPrivate::setSaveFilePath(const QString &value) {
    mutex.lock();
    saveFilePath = value;
    mutex.unlock();
}

cv::Mat ImageStackerPrivate::getWorkingImage() const {
    mutex.lock();
    cv::Mat image = workingImage.clone();
    mutex.unlock();

    return image;
}
void ImageStackerPrivate::setWorkingImage(const cv::Mat &value) {
    mutex.lock();
    workingImage = value.clone();
    mutex.unlock();
}

cv::Mat ImageStackerPrivate::getRefImage() const {
    mutex.lock();
    cv::Mat image = refImage.clone();
    mutex.unlock();

    return image;
}
void ImageStackerPrivate::setRefImage(const cv::Mat &value) {
    mutex.lock();
    refImage = value.clone();
    mutex.unlock();
}

cv::Mat ImageStackerPrivate::getFinalImage() const {
    mutex.lock();
    cv::Mat image = finalImage.clone();
    mutex.unlock();

    return image;
}
void ImageStackerPrivate::setFinalImage(const cv::Mat &value) {
    mutex.lock();
    finalImage = value.clone();
    mutex.unlock();
}
