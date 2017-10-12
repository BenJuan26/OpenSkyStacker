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

    if (use_bias_)       StackBias();
    if (use_darks_)      StackDarks();
    if (use_dark_flats_) StackDarkFlats();
    if (use_flats_)      StackFlats();

    emit UpdateProgress(tr("Reading light frame 1 of %n", "",
            target_image_file_names_.length() + 1),
            100*current_operation_/total_operations_);
    current_operation_++;

    ref_image_ = GetCalibratedImage(ref_image_file_name_,
            use_darks_ ? master_dark_ : cv::Mat(),
            use_flats_ ? master_flat_ : cv::Mat(),
            use_bias_  ? master_bias_ : cv::Mat());
    working_image_ = ref_image_.clone();

    QString message;
    int totalValidImages = 1;

    for (int k = 0; k < target_image_file_names_.length() && !cancel_; k++) {
        // ---------------- LOAD -----------------
        message = tr("Reading light frame %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        qDebug() << message;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        cv::Mat targetImage = GetCalibratedImage(target_image_file_names_.at(k),
                use_darks_ ? master_dark_ : cv::Mat(),
                use_flats_ ? master_flat_ : cv::Mat(),
                use_bias_  ? master_bias_ : cv::Mat());

        // -------------- ALIGNMENT ---------------
        message = tr("Aligning image %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        int ok = 0;
        cv::Mat targetAligned = GenerateAlignedImage(ref_image_, targetImage, tolerance, &ok);

        if (cancel_) return;

        if (ok != 0)
            continue;

        // -------------- STACKING ---------------
        message = tr("Stacking image %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        qDebug() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        cv::add(working_image_, targetAligned, working_image_, cv::noArray(), CV_32F);
        totalValidImages++;
    }

    if (cancel_) return;
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

void ImageStacker::StackDarks()
{
    cv::Mat dark1;
    bool raw = GetImageType(dark_frame_file_names_.at(0)) == RAW_IMAGE;
    if (raw) {
        dark1 = GetBayerMatrix(dark_frame_file_names_.at(0));
    } else {
        dark1 = ReadImage(dark_frame_file_names_.at(0));
    }
    if (use_bias_) dark1 -= master_bias_;

    cv::Mat result;
    dark1.convertTo(result, CV_32F);

    QString message;

    for (int i = 1; i < dark_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat dark;
        if (raw) {
            dark = GetBayerMatrix(dark_frame_file_names_.at(i));
        } else {
            dark = ReadImage(dark_frame_file_names_.at(i));
        }

        message = tr("Stacking dark frame %1 of %2").arg(QString::number(i+1), QString::number(dark_frame_file_names_.length()));
        qDebug() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_) dark -= master_bias_;
        cv::add(result, dark, result, cv::noArray(), CV_32F);
    }

    result /= dark_frame_file_names_.length();

    if (raw)
        result.convertTo(master_dark_, CV_16U);
    else
        master_dark_ = result.clone();
}

void ImageStacker::StackDarkFlats()
{
    cv::Mat darkFlat1;
    bool raw = GetImageType(dark_flat_frame_file_names_.at(0)) == RAW_IMAGE;
    if (raw) {
        darkFlat1 = GetBayerMatrix(dark_flat_frame_file_names_.at(0));
    } else {
        darkFlat1 = ReadImage(dark_flat_frame_file_names_.at(0));
    }
    if (use_bias_) darkFlat1 -= master_bias_;

    cv::Mat result;
    darkFlat1.convertTo(result, CV_32F);

    QString message;

    for (int i = 1; i < dark_flat_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat dark;
        if (raw) {
            dark = GetBayerMatrix(dark_flat_frame_file_names_.at(i));
        } else {
            dark = ReadImage(dark_flat_frame_file_names_.at(i));
        }

        message = tr("Stacking dark flat frame %1 of %2").arg(QString::number(i+1), QString::number(dark_flat_frame_file_names_.length()));
        qDebug() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_) dark -= master_bias_;
        cv::add(result, dark, result, cv::noArray(), CV_32F);
    }

    result /= dark_flat_frame_file_names_.length();

    if (raw)
        result.convertTo(master_dark_flat_, CV_16U);
    else
        master_dark_flat_ = result.clone();
}

void ImageStacker::StackFlats()
{
    // most algorithms compute the median, but we will stick with mean for now
    cv::Mat flat1;
    bool raw = GetImageType(flat_frame_file_names_.at(0)) == RAW_IMAGE;
    if (raw) {
        flat1 = GetBayerMatrix(flat_frame_file_names_.at(0));
    } else {
        flat1 = ReadImage(flat_frame_file_names_.at(0));
    }
    if (use_bias_) flat1 -= master_bias_;
    if (use_dark_flats_) flat1 -= master_dark_flat_;

    cv::Mat result;
    flat1.convertTo(result, CV_32F);

    QString message;

    for (int i = 1; i < flat_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat flat;
        if (raw) {
            flat = GetBayerMatrix(flat_frame_file_names_.at(i));
        } else {
            flat = ReadImage(flat_frame_file_names_.at(i));
        }

        message = tr("Stacking flat frame %1 of %2").arg(QString::number(i+1), QString::number(flat_frame_file_names_.length()));
        qDebug() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_) flat -= master_bias_;
        if (use_dark_flats_) flat -= master_dark_flat_;
        cv::add(result, flat, result, cv::noArray(), CV_32F);
    }

    result /= flat_frame_file_names_.length();

    // scale the flat frame so that the average value is 1.0
    // since we're dividing, flat values darker than the average value will brighten the image
    //  and values brighter than the average will darken the image, flattening the field
    float avg = cv::mean(result)[0];

    master_flat_ = result / avg;
}

void ImageStacker::StackBias()
{
    cv::Mat bias1;
    bool raw = GetImageType(bias_frame_file_names_.at(0)) == RAW_IMAGE;
    if (raw) {
        bias1 = GetBayerMatrix(bias_frame_file_names_.at(0));
    } else {
        bias1 = ReadImage(bias_frame_file_names_.at(0));
    }
    cv::Mat result;
    bias1.convertTo(result, CV_32F);

    QString message;

    for (int i = 1; i < bias_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat bias;
        if (raw) {
            bias = GetBayerMatrix(bias_frame_file_names_.at(i));
        } else {
            bias = ReadImage(bias_frame_file_names_.at(i));
        }
        message = tr("Stacking bias frame %1 of %2").arg(QString::number(i+1), QString::number(bias_frame_file_names_.length()));
        qDebug() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        cv::add(result, bias, result, cv::noArray(), CV_32F);
    }

    result /= bias_frame_file_names_.length();

    if (raw)
        result.convertTo(master_bias_, CV_16U);
    else
        master_bias_ = result.clone();
}

// derived from FOCAS mktransform.c
cv::Mat ImageStacker::GenerateAlignedImage(cv::Mat ref, cv::Mat target, int tolerance, int *ok) {
    StarDetector sd;
    std::vector<Star> List1 = sd.GetStars(ref, tolerance);
    std::vector<Star> List2 = sd.GetStars(target, tolerance);

    std::vector<Triangle> List_triangA = GenerateTriangleList(List1);
    std::vector<Triangle> List_triangB = GenerateTriangleList(List2);

    int nobjs = 40;

    int k = 0;
    std::vector< std::vector<int> > matches = FindMatches(nobjs, &k, List_triangA, List_triangB);
    std::vector< std::vector<float> > transformVec = FindTransform(matches, k, List1, List2, ok);

    if (ok && *ok != 0)
        return target;

    cv::Mat matTransform(2,3,CV_32F);
    for(int i = 0; i < 2; i++)
        for (int j = 0; j < 3; j++)
            matTransform.at<float>(i,j) = transformVec[i][j];

    warpAffine(target, target, matTransform, target.size(), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP);

    return target;
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
