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
        cancel_(false)
{

}

ImageStacker::~ImageStacker() {}

void ImageStacker::Process(int tolerance, int threads) {
    Q_D(ImageStacker);
    d->Process(tolerance, threads);
}

void ImageStacker::ReadQImage(QString filename)
{
    Q_D(ImageStacker);
    d->ReadQImage(filename);
}

void ImageStacker::detectStars(QString filename, int threshold)
{
    Q_D(ImageStacker);
    d->detectStars(filename, threshold);
}

// GETTER / SETTER

bool ImageStacker::GetUseFlats() const
{
    Q_D(const ImageStacker);
    return d->GetUseFlats();
}

void ImageStacker::SetUseFlats(bool value)
{
    Q_D(ImageStacker);
    d->SetUseFlats(value);
}

bool ImageStacker::GetUseDarkFlats() const
{
    Q_D(const ImageStacker);
    return d->GetUseDarkFlats();
}

void ImageStacker::SetUseDarkFlats(bool value)
{
    Q_D(ImageStacker);
    d->SetUseDarkFlats(value);
}

bool ImageStacker::GetUseDarks() const
{
    Q_D(const ImageStacker);
    return d->GetUseDarks();
}

void ImageStacker::SetUseDarks(bool value)
{
    Q_D(ImageStacker);
    d->SetUseDarks(value);
}

bool ImageStacker::GetUseBias() const
{
    Q_D(const ImageStacker);
    return d->GetUseBias();
}

void ImageStacker::SetUseBias(bool value)
{
    Q_D(ImageStacker);
    d->SetUseBias(value);
}

QString ImageStacker::GetRefImageFileName() const {
    Q_D(const ImageStacker);
    return d->GetRefImageFileName();
}
void ImageStacker::SetRefImageFileName(const QString &value) {
    Q_D(ImageStacker);
    d->SetRefImageFileName(value);
}

QStringList ImageStacker::GetTargetImageFileNames() const {
    Q_D(const ImageStacker);
    return d->GetTargetImageFileNames();
}
void ImageStacker::SetTargetImageFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->SetTargetImageFileNames(value);
}

QStringList ImageStacker::GetDarkFrameFileNames() const {
    Q_D(const ImageStacker);
    return d->GetDarkFrameFileNames();
}
void ImageStacker::SetDarkFrameFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->SetDarkFrameFileNames(value);
}

QStringList ImageStacker::GetDarkFlatFrameFileNames() const {
    Q_D(const ImageStacker);
    return d->GetDarkFlatFrameFileNames();
}
void ImageStacker::SetDarkFlatFrameFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->SetDarkFlatFrameFileNames(value);
}

QStringList ImageStacker::GetFlatFrameFileNames() const {
    Q_D(const ImageStacker);
    return d->GetFlatFrameFileNames();
}
void ImageStacker::SetFlatFrameFileNames(const QStringList &value) {
    Q_D(ImageStacker);
    d->SetFlatFrameFileNames(value);
}

QStringList ImageStacker::GetBiasFrameFileNames() const
{
    Q_D(const ImageStacker);
    return d->GetBiasFrameFileNames();
}

void ImageStacker::SetBiasFrameFileNames(const QStringList &value)
{
    Q_D(ImageStacker);
    d->SetBiasFrameFileNames(value);
}

QString ImageStacker::GetSaveFilePath() const {
    Q_D(const ImageStacker);
    return d->GetSaveFilePath();
}
void ImageStacker::SetSaveFilePath(const QString &value) {
    Q_D(ImageStacker);
    d->SetSaveFilePath(value);
}

cv::Mat ImageStacker::GetWorkingImage() const {
    Q_D(const ImageStacker);
    return d->GetWorkingImage();
}
void ImageStacker::SetWorkingImage(const cv::Mat &value) {
    Q_D(ImageStacker);
    d->SetWorkingImage(value);
}

cv::Mat ImageStacker::GetRefImage() const {
    Q_D(const ImageStacker);
    return d->GetRefImage();
}
void ImageStacker::SetRefImage(const cv::Mat &value) {
    Q_D(ImageStacker);
    d->SetRefImage(value);
}

cv::Mat ImageStacker::GetFinalImage() const {
    Q_D(const ImageStacker);
    return d->GetFinalImage();
}
void ImageStacker::SetFinalImage(const cv::Mat &value) {
    Q_D(ImageStacker);
    d->SetFinalImage(value);
}








ImageStackerPrivate::ImageStackerPrivate(ImageStacker *parent) : q_ptr(parent)
{

}

int ImageStackerPrivate::GetTotalOperations()
{
    int ops = target_image_file_names_.length() + 4;

    if (use_bias_)       ops += 1;
    if (use_darks_)      ops += 1;
    if (use_dark_flats_) ops += 1;
    if (use_flats_)      ops += 1;

    return ops;
}

void ImageStackerPrivate::Process(int tolerance, int threads) {
    Q_Q(ImageStacker);
    time_t now;
    time(&now);

    qDebug() << "Path:" << save_file_path_;

    ImageType refType = GetImageType(ref_image_file_name_);

    for (int i = 0; i < target_image_file_names_.length(); i++) {
        if (GetImageType(target_image_file_names_.at(i)) != refType) {
            emit q->ProcessingError(QObject::tr("Images must be the same type."));
            return;
        }
    }

    q->cancel_ = false;
    current_operation_ = 1;
    total_operations_ = GetTotalOperations();
    emit q->UpdateProgress(QObject::tr("Checking image sizes..."), 100 * current_operation_++ / total_operations_);

    int err = ValidateImageSizes();
    if (err) {
        emit q->ProcessingError("Images must all be the same size.");
        return;
    }

    cv::Mat masterDark, masterDarkFlat, masterFlat, masterBias;

    if (use_bias_) {
        emit q->UpdateProgress(QObject::tr("Stacking bias frames..."), 100 * current_operation_++ / total_operations_);
        masterBias = StackBias(bias_frame_file_names_);
    }
    if (use_darks_) {
        emit q->UpdateProgress(QObject::tr("Stacking dark frames..."), 100 * current_operation_++ / total_operations_);
        masterDark = StackDarks(dark_frame_file_names_, masterBias);
    }
    if (use_dark_flats_) {
        emit q->UpdateProgress(QObject::tr("Stacking dark flat frames..."), 100 * current_operation_++ / total_operations_);
        masterDarkFlat = StackDarkFlats(dark_flat_frame_file_names_, masterBias);
    }
    if (use_flats_) {
        emit q->UpdateProgress(QObject::tr("Stacking flat frames..."), 100 * current_operation_++ / total_operations_);
        masterFlat = StackFlats(flat_frame_file_names_, masterDarkFlat, masterBias);
    }

    emit q->UpdateProgress(QObject::tr("Stacking light frames..."), 100 * current_operation_++ / total_operations_);

    ref_image_ = GetCalibratedImage(ref_image_file_name_, masterDark , masterFlat, masterBias);
    working_image_ = ref_image_.clone();

    int totalValidImages = 1;
    if (threads >= target_image_file_names_.length())
        threads = target_image_file_names_.length() - 1;

    StackingParams params;
    params.lights = target_image_file_names_;
    params.ref = ref_image_;
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
        QFuture<StackingResult> future = QtConcurrent::run(ProcessConcurrent, params, c);
        futures.push_back(future);
    }

    bool done = false;
    while (!done) {
        done = true;
        int op = current_operation_;
        for (int i = 0; i < futures.size(); i++) {
            QFuture<StackingResult> future = futures.at(i);
            op += *completes.at(i);
            done = done && future.isFinished();
        }

        emit q->UpdateProgress(QObject::tr("Stacking light frames..."), 100 * op / total_operations_);
        QThread::msleep(30);
    }

    for (QFuture<StackingResult> future : futures) {
        working_image_ += future.result().image;
        totalValidImages += future.result().totalValidImages;
    }

    if (totalValidImages < 2) {
        emit q->ProcessingError(QObject::tr("No images could be aligned to the reference image. Try using a lower tolerance."));
        return;
    }

    working_image_ /= totalValidImages;

    // LibRaw works in RGB while OpenCV works in BGR
    if (GetImageType(ref_image_file_name_) == RAW_IMAGE)
        cv::cvtColor(working_image_, working_image_, CV_RGB2BGR);

    time_t doneStacking;
    time(&doneStacking);

    qDebug() << "Stacking took" << difftime(doneStacking, now) << "seconds";

    emit q->Finished(working_image_, QObject::tr("Stacking completed in %1 seconds.").arg(difftime(doneStacking, now)));
}

void ImageStackerPrivate::ReadQImage(QString filename)
{
    Q_Q(ImageStacker);
    cv::Mat image = ReadImage(filename);

    double min, max;
    cv::minMaxLoc(image, &min, &max);

    // stretch intensity levels
    image *= (1.0/max);

    emit q->QImageReady(Mat2QImage(image));
}

void ImageStackerPrivate::detectStars(QString filename, int threshold)
{
    Q_Q(ImageStacker);
    cv::Mat image = ReadImage(filename);
    StarDetector sd;
    std::vector<Star> list = sd.GetStars(image, threshold);

    emit q->doneDetectingStars(list.size());
}

int ImageStackerPrivate::ValidateImageSizes()
{
    LibRaw processor;

    // params for raw processing
    processor.imgdata.params.use_auto_wb = 0;
    processor.imgdata.params.use_camera_wb = 1;
    processor.imgdata.params.no_auto_bright = 1;
    processor.imgdata.params.output_bps = 16;

    QFileInfo info(ref_image_file_name_);
    QString ext = info.completeSuffix();
    int refWidth;
    int refHeight;

    if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
        processor.open_file(ref_image_file_name_.toUtf8().constData());
        refWidth = processor.imgdata.sizes.width;
        refHeight = processor.imgdata.sizes.height;
        processor.free_image();
    } else {
        cv::Mat ref = ReadImage(ref_image_file_name_);
        refWidth = ref.cols;
        refHeight = ref.rows;
    }

    for (int i = 0; i < target_image_file_names_.length(); i++) {
        QString filename = target_image_file_names_.at(i);

        int width;
        int height;

        if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
            processor.open_file(filename.toUtf8().constData());
            width = processor.imgdata.sizes.width;
            height = processor.imgdata.sizes.height;
            processor.free_image();
        } else {
            cv::Mat ref = ReadImage(filename);
            width = ref.cols;
            height = ref.rows;
        }

        if (width != refWidth ||  height != refHeight) {
            return -1;
        }
    }

    if (use_bias_) {
        for (int i = 0; i < bias_frame_file_names_.length(); i++) {
            QString filename = bias_frame_file_names_.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = ReadImage(filename);
                width = ref.cols;
                height = ref.rows;
            }

            if (width != refWidth ||  height != refHeight) {
                return -1;
            }
        }
    }

    if (use_darks_) {
        for (int i = 0; i < dark_frame_file_names_.length(); i++) {
            QString filename = dark_frame_file_names_.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = ReadImage(filename);
                width = ref.cols;
                height = ref.rows;
            }

            if (width != refWidth ||  height != refHeight) {
                return -1;
            }
        }
    }

    if (use_dark_flats_) {
        for (int i = 0; i < dark_flat_frame_file_names_.length(); i++) {
            QString filename = dark_flat_frame_file_names_.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = ReadImage(filename);
                width = ref.cols;
                height = ref.rows;
            }

            if (width != refWidth ||  height != refHeight) {
                return -1;
            }
        }
    }

    if (use_flats_) {
        for (int i = 0; i < flat_frame_file_names_.length(); i++) {
            QString filename = flat_frame_file_names_.at(i);

            int width;
            int height;

            if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
                processor.open_file(filename.toUtf8().constData());
                width = processor.imgdata.sizes.width;
                height = processor.imgdata.sizes.height;
                processor.free_image();
            } else {
                cv::Mat ref = ReadImage(filename);
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

bool ImageStackerPrivate::GetUseFlats() const
{
    mutex_.lock();
    bool value = use_flats_;
    mutex_.unlock();

    return value;
}

void ImageStackerPrivate::SetUseFlats(bool value)
{
    mutex_.lock();
    use_flats_ = value;
    mutex_.unlock();
}

bool ImageStackerPrivate::GetUseDarkFlats() const
{
    mutex_.lock();
    bool value = use_dark_flats_;
    mutex_.unlock();

    return value;
}

void ImageStackerPrivate::SetUseDarkFlats(bool value)
{
    mutex_.lock();
    use_dark_flats_ = value;
    mutex_.unlock();
}

bool ImageStackerPrivate::GetUseDarks() const
{
    mutex_.lock();
    bool value = use_darks_;
    mutex_.unlock();

    return value;
}

void ImageStackerPrivate::SetUseDarks(bool value)
{
    mutex_.lock();
    use_darks_ = value;
    mutex_.unlock();
}

bool ImageStackerPrivate::GetUseBias() const
{
    mutex_.lock();
    bool value = use_bias_;
    mutex_.unlock();

    return value;
}

void ImageStackerPrivate::SetUseBias(bool value)
{
    mutex_.lock();
    use_bias_ = value;
    mutex_.unlock();
}

QString ImageStackerPrivate::GetRefImageFileName() const {
    mutex_.lock();
    QString string = ref_image_file_name_;
    mutex_.unlock();

    return string;
}
void ImageStackerPrivate::SetRefImageFileName(const QString &value) {
    mutex_.lock();
    ref_image_file_name_ = value;
    mutex_.unlock();
}

QStringList ImageStackerPrivate::GetTargetImageFileNames() const {
    mutex_.lock();
    QStringList list = target_image_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStackerPrivate::SetTargetImageFileNames(const QStringList &value) {
    mutex_.lock();
    target_image_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStackerPrivate::GetDarkFrameFileNames() const {
    mutex_.lock();
    QStringList list = dark_frame_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStackerPrivate::SetDarkFrameFileNames(const QStringList &value) {
    mutex_.lock();
    dark_frame_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStackerPrivate::GetDarkFlatFrameFileNames() const {
    mutex_.lock();
    QStringList list = dark_flat_frame_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStackerPrivate::SetDarkFlatFrameFileNames(const QStringList &value) {
    mutex_.lock();
    dark_flat_frame_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStackerPrivate::GetFlatFrameFileNames() const {
    mutex_.lock();
    QStringList list = flat_frame_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStackerPrivate::SetFlatFrameFileNames(const QStringList &value) {
    mutex_.lock();
    flat_frame_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStackerPrivate::GetBiasFrameFileNames() const
{
    mutex_.lock();
    QStringList list = bias_frame_file_names_;
    mutex_.unlock();

    return list;
}

void ImageStackerPrivate::SetBiasFrameFileNames(const QStringList &value)
{
    mutex_.lock();
    bias_frame_file_names_ = value;
    mutex_.unlock();
}

QString ImageStackerPrivate::GetSaveFilePath() const {
    mutex_.lock();
    QString path = save_file_path_;
    mutex_.unlock();

    return path;
}
void ImageStackerPrivate::SetSaveFilePath(const QString &value) {
    mutex_.lock();
    save_file_path_ = value;
    mutex_.unlock();
}

cv::Mat ImageStackerPrivate::GetWorkingImage() const {
    mutex_.lock();
    cv::Mat image = working_image_.clone();
    mutex_.unlock();

    return image;
}
void ImageStackerPrivate::SetWorkingImage(const cv::Mat &value) {
    mutex_.lock();
    working_image_ = value.clone();
    mutex_.unlock();
}

cv::Mat ImageStackerPrivate::GetRefImage() const {
    mutex_.lock();
    cv::Mat image = ref_image_.clone();
    mutex_.unlock();

    return image;
}
void ImageStackerPrivate::SetRefImage(const cv::Mat &value) {
    mutex_.lock();
    ref_image_ = value.clone();
    mutex_.unlock();
}

cv::Mat ImageStackerPrivate::GetFinalImage() const {
    mutex_.lock();
    cv::Mat image = final_image_.clone();
    mutex_.unlock();

    return image;
}
void ImageStackerPrivate::SetFinalImage(const cv::Mat &value) {
    mutex_.lock();
    final_image_ = value.clone();
    mutex_.unlock();
}
