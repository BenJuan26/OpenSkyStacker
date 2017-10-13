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
    int ops = target_image_file_names_.length() * 2 + 1;

    if (use_bias_)       ops += bias_frame_file_names_.length();
    if (use_darks_)      ops += dark_frame_file_names_.length();
    if (use_dark_flats_) ops += dark_flat_frame_file_names_.length();
    if (use_flats_)      ops += flat_frame_file_names_.length();

    return ops;
}

void ImageStacker::Process(int tolerance) {
    time_t now;
    time(&now);

    ImageType refType = GetImageType(ref_image_file_name_);

    for (int i = 0; i < target_image_file_names_.length(); i++) {
        if (GetImageType(target_image_file_names_.at(i)) != refType) {
            emit ProcessingError(tr("Images must be the same type."));
            return;
        }
    }

    cancel_ = false;
    emit UpdateProgress(tr("Checking image sizes"), 0);

    int err = ValidateImageSizes();
    if (err) {
        emit ProcessingError("Images must all be the same size.");
        return;
    }

    current_operation_ = 0;
    total_operations_ = GetTotalOperations();

    cv::Mat masterDark, masterDarkFlat, masterFlat, masterBias;

    if (use_bias_)       masterBias = StackBias(bias_frame_file_names_);
    if (use_darks_)      masterDark = StackDarks(dark_frame_file_names_, masterBias);
    if (use_dark_flats_) masterDarkFlat = StackDarkFlats(dark_flat_frame_file_names_, masterBias);
    if (use_flats_)      masterFlat = StackFlats(flat_frame_file_names_, masterDarkFlat, masterBias);

    emit UpdateProgress(tr("Reading light frame 1 of %n", "",
            target_image_file_names_.length() + 1),
            100*current_operation_/total_operations_);
    current_operation_++;

    ref_image_ = GetCalibratedImage(ref_image_file_name_, masterDark , masterFlat, masterBias);
    working_image_ = ref_image_.clone();

    int totalValidImages = 1;

    StackingParams params;
    params.lights = target_image_file_names_;
    params.ref = ref_image_;
    params.masterDark = masterDark;
    params.masterFlat = masterFlat;
    params.masterBias = masterBias;
    params.tolerance = tolerance;
    params.threadIndex = 0;
    params.totalThreads = 4;
    QFuture<StackingResult> future1 = QtConcurrent::run(ProcessConcurrent, params);

    params.threadIndex = 1;
    QFuture<StackingResult> future2 = QtConcurrent::run(ProcessConcurrent, params);

    params.threadIndex = 2;
    QFuture<StackingResult> future3 = QtConcurrent::run(ProcessConcurrent, params);

    params.threadIndex = 3;
    QFuture<StackingResult> future4 = QtConcurrent::run(ProcessConcurrent, params);

    bool done = false;
    while (!done) {
        done = done || future1.isFinished();
        done = done || future2.isFinished();
        done = done || future3.isFinished();
        done = done || future4.isFinished();
        QThread::sleep(1);
    }

    working_image_ += future1.result().image;
    working_image_ += future2.result().image;
    working_image_ += future3.result().image;
    working_image_ += future4.result().image;

    totalValidImages += future1.result().totalValidImages;
    totalValidImages += future2.result().totalValidImages;
    totalValidImages += future3.result().totalValidImages;
    totalValidImages += future4.result().totalValidImages;

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

    emit Finished(working_image_, tr("Stacking completed"));
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

    qDebug() << "refImage:" << refWidth << refHeight;

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

            qDebug() << "bias" << i << ":" << width << height;

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

            qDebug() << "dark" << i << ":" << width << height;

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

            qDebug() << "dark flat" << i << ":" << width << height;

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

            qDebug() << "flat" << i << ":" << width << height;

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
