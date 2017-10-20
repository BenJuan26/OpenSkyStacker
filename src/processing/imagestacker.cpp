#include "imagestacker.h"

using namespace openskystacker;
using namespace CCfits;
using namespace easyexif;

ImageStacker::ImageStacker(QObject *parent) : QObject(parent)
{
    cancel_ = false;
}

int ImageStacker::GetTotalOperations()
{
    int ops = target_image_file_names_.length() + 4;

    if (use_bias_)       ops += 1;
    if (use_darks_)      ops += 1;
    if (use_dark_flats_) ops += 1;
    if (use_flats_)      ops += 1;

    return ops;
}

void ImageStacker::Process(int tolerance, int threads) {
    time_t now;
    time(&now);

    qDebug() << "Path:" << save_file_path_;

    ImageType refType = GetImageType(ref_image_file_name_);

    for (int i = 0; i < target_image_file_names_.length(); i++) {
        if (GetImageType(target_image_file_names_.at(i)) != refType) {
            emit ProcessingError(tr("Images must be the same type."));
            return;
        }
    }

    cancel_ = false;
    current_operation_ = 1;
    total_operations_ = GetTotalOperations();
    emit UpdateProgress(tr("Checking image sizes..."), 100 * current_operation_++ / total_operations_);

    int err = ValidateImageSizes();
    if (err) {
        emit ProcessingError("Images must all be the same size.");
        return;
    }

    cv::Mat masterDark, masterDarkFlat, masterFlat, masterBias;

    if (use_bias_) {
        emit UpdateProgress(tr("Stacking bias frames..."), 100 * current_operation_++ / total_operations_);
        masterBias = StackBias(bias_frame_file_names_);
    }
    if (use_darks_) {
        emit UpdateProgress(tr("Stacking dark frames..."), 100 * current_operation_++ / total_operations_);
        masterDark = StackDarks(dark_frame_file_names_, masterBias);
    }
    if (use_dark_flats_) {
        emit UpdateProgress(tr("Stacking dark flat frames..."), 100 * current_operation_++ / total_operations_);
        masterDarkFlat = StackDarkFlats(dark_flat_frame_file_names_, masterBias);
    }
    if (use_flats_) {
        emit UpdateProgress(tr("Stacking flat frames..."), 100 * current_operation_++ / total_operations_);
        masterFlat = StackFlats(flat_frame_file_names_, masterDarkFlat, masterBias);
    }

    emit UpdateProgress(tr("Stacking light frames..."), 100 * current_operation_++ / total_operations_);

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

        emit UpdateProgress(tr("Stacking light frames..."), 100 * op / total_operations_);
        QThread::msleep(30);
    }

    for (QFuture<StackingResult> future : futures) {
        working_image_ += future.result().image;
        totalValidImages += future.result().totalValidImages;
    }

    if (totalValidImages < 2) {
        emit ProcessingError(tr("No images could be aligned to the reference image. Try using a lower tolerance."));
        return;
    }

    working_image_ /= totalValidImages;

    // LibRaw works in RGB while OpenCV works in BGR
    if (GetImageType(ref_image_file_name_) == RAW_IMAGE)
        cv::cvtColor(working_image_, working_image_, CV_RGB2BGR);

    time_t doneStacking;
    time(&doneStacking);

    qDebug() << "Stacking took" << difftime(doneStacking, now) << "seconds";

    emit Finished(working_image_, tr("Stacking completed in %1 seconds.").arg(difftime(doneStacking, now)));
}

void ImageStacker::ReadQImage(QString filename)
{
    cv::Mat image = ReadImage(filename);

    double min, max;
    cv::minMaxLoc(image, &min, &max);

    // stretch intensity levels
    image *= (1.0/max);

    emit QImageReady(Mat2QImage(image));
}

void ImageStacker::detectStars(QString filename, int threshold)
{
    cv::Mat image = ReadImage(filename);
    StarDetector sd;
    std::vector<Star> list = sd.GetStars(image, threshold);

    emit doneDetectingStars(list.size());
}

int ImageStacker::ValidateImageSizes()
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

bool ImageStacker::GetUseFlats() const
{
    mutex_.lock();
    bool value = use_flats_;
    mutex_.unlock();

    return value;
}

void ImageStacker::SetUseFlats(bool value)
{
    mutex_.lock();
    use_flats_ = value;
    mutex_.unlock();
}

bool ImageStacker::GetUseDarkFlats() const
{
    mutex_.lock();
    bool value = use_dark_flats_;
    mutex_.unlock();

    return value;
}

void ImageStacker::SetUseDarkFlats(bool value)
{
    mutex_.lock();
    use_dark_flats_ = value;
    mutex_.unlock();
}

bool ImageStacker::GetUseDarks() const
{
    mutex_.lock();
    bool value = use_darks_;
    mutex_.unlock();

    return value;
}

void ImageStacker::SetUseDarks(bool value)
{
    mutex_.lock();
    use_darks_ = value;
    mutex_.unlock();
}

bool ImageStacker::GetUseBias() const
{
    mutex_.lock();
    bool value = use_bias_;
    mutex_.unlock();

    return value;
}

void ImageStacker::SetUseBias(bool value)
{
    mutex_.lock();
    use_bias_ = value;
    mutex_.unlock();
}

QString ImageStacker::GetRefImageFileName() const {
    mutex_.lock();
    QString string = ref_image_file_name_;
    mutex_.unlock();

    return string;
}
void ImageStacker::SetRefImageFileName(const QString &value) {
    mutex_.lock();
    ref_image_file_name_ = value;
    mutex_.unlock();
}

QStringList ImageStacker::GetTargetImageFileNames() const {
    mutex_.lock();
    QStringList list = target_image_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStacker::SetTargetImageFileNames(const QStringList &value) {
    mutex_.lock();
    target_image_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStacker::GetDarkFrameFileNames() const {
    mutex_.lock();
    QStringList list = dark_frame_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStacker::SetDarkFrameFileNames(const QStringList &value) {
    mutex_.lock();
    dark_frame_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStacker::GetDarkFlatFrameFileNames() const {
    mutex_.lock();
    QStringList list = dark_flat_frame_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStacker::SetDarkFlatFrameFileNames(const QStringList &value) {
    mutex_.lock();
    dark_flat_frame_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStacker::GetFlatFrameFileNames() const {
    mutex_.lock();
    QStringList list = flat_frame_file_names_;
    mutex_.unlock();

    return list;
}
void ImageStacker::SetFlatFrameFileNames(const QStringList &value) {
    mutex_.lock();
    flat_frame_file_names_ = value;
    mutex_.unlock();
}

QStringList ImageStacker::GetBiasFrameFileNames() const
{
    mutex_.lock();
    QStringList list = bias_frame_file_names_;
    mutex_.unlock();

    return list;
}

void ImageStacker::SetBiasFrameFileNames(const QStringList &value)
{
    mutex_.lock();
    bias_frame_file_names_ = value;
    mutex_.unlock();
}

QString ImageStacker::GetSaveFilePath() const {
    mutex_.lock();
    QString path = save_file_path_;
    mutex_.unlock();

    return path;
}
void ImageStacker::SetSaveFilePath(const QString &value) {
    mutex_.lock();
    save_file_path_ = value;
    mutex_.unlock();
}

cv::Mat ImageStacker::GetWorkingImage() const {
    mutex_.lock();
    cv::Mat image = working_image_.clone();
    mutex_.unlock();

    return image;
}
void ImageStacker::SetWorkingImage(const cv::Mat &value) {
    mutex_.lock();
    working_image_ = value.clone();
    mutex_.unlock();
}

cv::Mat ImageStacker::GetRefImage() const {
    mutex_.lock();
    cv::Mat image = ref_image_.clone();
    mutex_.unlock();

    return image;
}
void ImageStacker::SetRefImage(const cv::Mat &value) {
    mutex_.lock();
    ref_image_ = value.clone();
    mutex_.unlock();
}

cv::Mat ImageStacker::GetFinalImage() const {
    mutex_.lock();
    cv::Mat image = final_image_.clone();
    mutex_.unlock();

    return image;
}
void ImageStacker::SetFinalImage(const cv::Mat &value) {
    mutex_.lock();
    final_image_ = value.clone();
    mutex_.unlock();
}
