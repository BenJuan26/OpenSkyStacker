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
#include <libraw.h>

const std::vector<QString> ImageStacker::RAW_EXTENSIONS = {"3fr", "ari", "arw", "bay", "crw", "cr2",
        "cap", "data", "dcs", "dcr", "dng", "drf", "eip", "erf", "fff", "gpr", "iiq", "k25", "kdc",
        "mdc", "mef", "mos", "mrw", "nef", "nrw", "obm", "orf", "pef", "ptx", "pxn", "r3d", "raf",
        "raw", "rwl", "rw2", "rwz", "sr2", "srf", "srw", "x3f"};

ImageStacker::ImageStacker(QObject *parent) : QObject(parent)
{
    cancel_ = false;
    bits_per_channel_ = BITS_32;
}

ImageRecord* ImageStacker::GetImageRecord(QString filename)
{
    LibRaw processor;

    processor.open_file(filename.toUtf8().constData());

    libraw_imgother_t other = processor.imgdata.other;

    ImageRecord *record = new ImageRecord();
    record->SetFilename(filename);
    record->SetIso(other.iso_speed);
    record->SetShutter(other.shutter);
    record->SetTimestamp(other.timestamp);
    record->SetWidth(processor.imgdata.sizes.width);
    record->SetHeight(processor.imgdata.sizes.height);

    return record;
}

void ImageStacker::ProcessNonRaw() {
    cancel_ = false;
    emit UpdateProgress(tr("Checking image sizes"), 0);

    int err = ValidateImageSizes();
    if (err) {
        emit ProcessingError("Images must all be the same size.");
        return;
    }

    current_operation_ = 0;
    total_operations_ = target_image_file_names_.length() * 2 + 1;

    if (use_bias_)       total_operations_ += bias_frame_file_names_.length();
    if (use_darks_)      total_operations_ += dark_frame_file_names_.length();
    if (use_dark_flats_) total_operations_ += dark_flat_frame_file_names_.length();
    if (use_flats_)      total_operations_ += flat_frame_file_names_.length();

    if (use_bias_)       StackBias();
    if (use_darks_)      StackDarks();
    if (use_dark_flats_) StackDarkFlats();
    if (use_flats_)      StackFlats();


    emit UpdateProgress(tr("Reading light frame 1 of %n", "",
            target_image_file_names_.length() + 1),
            100*current_operation_/total_operations_);
    current_operation_++;

    ref_image_ = ReadImage(ref_image_file_name_);

    if (use_bias_)  ref_image_ -= master_bias_;
    if (use_darks_) ref_image_ -= master_dark_;
    if (use_flats_) ref_image_ /= master_flat_;

    // 32-bit float no matter what for the working image
    ref_image_.convertTo(working_image_, CV_32F);

    QString message;
    int totalValidImages = 1;

    for (int k = 0; k < target_image_file_names_.length() && !cancel_; k++) {
        // ---------------- LOAD -----------------
        message = tr("Reading light frame %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        qDebug() << message;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        cv::Mat targetImage = ReadImage(target_image_file_names_.at(k));


        // ------------- CALIBRATION --------------
        message = tr("Calibrating light frame %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        qDebug() << message;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_)  targetImage -= master_bias_;
        if (use_darks_) targetImage -= master_dark_;
        if (use_flats_) targetImage /= master_flat_;


        // -------------- ALIGNMENT ---------------
        message = tr("Aligning image %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        int ok = 0;
        cv::Mat targetAligned = GenerateAlignedImage(ref_image_, targetImage, &ok);

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

    // only need to change the bit depth, no scaling
    if (bits_per_channel_ == BITS_16) {
        working_image_.convertTo(working_image_, CV_16U);
    }

    emit Finished(working_image_, tr("Stacking completed"));
}

bool ImageStacker::FileHasRawExtension(QString filename)
{
    QFileInfo info(filename);
    QString ext = info.completeSuffix();
    return std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end();
}

void ImageStacker::Process() {
    bool raw = FileHasRawExtension(ref_image_file_name_);
    if (raw) {
        for (int i = 0; i < target_image_file_names_.length(); i++) {
            if (!FileHasRawExtension(target_image_file_names_.at(i))) {
                emit ProcessingError(tr("Cannot mix raw and processed images."));
                return;
            }
        }

        ProcessRaw();
    } else {
        for (int i = 0; i < target_image_file_names_.length(); i++) {
            if (FileHasRawExtension(target_image_file_names_.at(i))) {
                emit ProcessingError(tr("Cannot mix raw and processed images."));
                return;
            }
        }

        ProcessNonRaw();
    }
}

void ImageStacker::ProcessRaw() {
    cancel_ = false;
    emit UpdateProgress(tr("Checking image sizes"), 0);

    int err = ValidateImageSizes();
    if (err) {
        emit ProcessingError("Images must all be the same size.");
        return;
    }

    current_operation_ = 0;
    total_operations_ = target_image_file_names_.length() * 2 + 1;

    if (use_bias_)       total_operations_ += bias_frame_file_names_.length();
    if (use_darks_)      total_operations_ += dark_frame_file_names_.length();
    if (use_dark_flats_) total_operations_ += dark_flat_frame_file_names_.length();
    if (use_flats_)      total_operations_ += flat_frame_file_names_.length();

    if (use_bias_)       StackBias();
    if (use_darks_)      StackDarks();
    if (use_dark_flats_) StackDarkFlats();
    if (use_flats_)      StackFlats();

    emit UpdateProgress(tr("Reading light frame 1 of %n", "",
            target_image_file_names_.length() + 1),
            100*current_operation_/total_operations_);
    current_operation_++;

    ref_image_ = ReadImage(ref_image_file_name_);

    if (use_bias_)  ref_image_ -= master_bias_;
    if (use_darks_) ref_image_ -= master_dark_;
    if (use_flats_) ref_image_ /= master_flat_;

    // 32-bit float no matter what for the working image
    ref_image_.convertTo(working_image_, CV_32F);

    QString message;
    int totalValidImages = 1;

    for (int k = 0; k < target_image_file_names_.length() && !cancel_; k++) {
        // ---------------- LOAD -----------------
        message = tr("Reading light frame %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        qInfo() << message;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        cv::Mat targetImage = ReadImage(target_image_file_names_.at(k));


        // ------------- CALIBRATION --------------
        message = tr("Calibrating light frame %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        qInfo() << message;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_)  targetImage -= master_bias_;
        if (use_darks_) targetImage -= master_dark_;
        if (use_flats_) targetImage /= master_flat_;


        // -------------- ALIGNMENT ---------------
        message = tr("Aligning image %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        int ok = 0;
        cv::Mat targetAligned = GenerateAlignedImage(ref_image_, targetImage, &ok);

        if (cancel_) return;

        if (ok != 0)
            continue;

        // -------------- STACKING ---------------
        message = tr("Stacking image %1 of %2").arg(QString::number(k+2), QString::number(target_image_file_names_.length() + 1));
        qInfo() << message;
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

    // only need to change the bit depth, no scaling
    if (bits_per_channel_ == BITS_16) {
        working_image_.convertTo(working_image_, CV_16U);
    }

    emit FinishedDialog(tr("Stacking completed"));
    emit Finished(working_image_);
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

cv::Mat ImageStacker::AverageImages(cv::Mat img1, cv::Mat img2) {
    cv::Mat result;

    switch (bits_per_channel_) {
    case BITS_16: {
        result = cv::Mat(img1.rows, img1.cols, CV_16UC3);
        for(int x = 0; x < img1.cols; x++) {
            for(int y = 0; y < img1.rows; y++) {

                cv::Vec<unsigned short, 3> pixel1 = img1.at< cv::Vec<unsigned short,3> >(y,x);
                unsigned short b1 = pixel1.val[0];
                unsigned short g1 = pixel1.val[1];
                unsigned short r1 = pixel1.val[2];

                cv::Vec<unsigned short,3> pixel2 = img2.at< cv::Vec<unsigned short,3> >(y,x);
                unsigned short b2 = pixel2.val[0];
                unsigned short g2 = pixel2.val[1];
                unsigned short r2 = pixel2.val[2];

                result.at< cv::Vec<unsigned short,3> >(y,x).val[0] = (b1 + b2) / 2;
                result.at< cv::Vec<unsigned short,3> >(y,x).val[1] = (g1 + g2) / 2;
                result.at< cv::Vec<unsigned short,3> >(y,x).val[2] = (r1 + r2) / 2;
            }
        }

        break;
    }

    case BITS_32: {
        result = cv::Mat(img1.rows, img1.cols, CV_32FC3);
        for(int x = 0; x < img1.cols; x++) {
            for(int y = 0; y < img1.rows; y++) {

                cv::Vec3f pixel1 = img1.at<cv::Vec3f>(y,x);
                float b1 = pixel1.val[0];
                float g1 = pixel1.val[1];
                float r1 = pixel1.val[2];

                cv::Vec3f pixel2 = img2.at<cv::Vec3f>(y,x);
                float b2 = pixel2.val[0];
                float g2 = pixel2.val[1];
                float r2 = pixel2.val[2];

                result.at<cv::Vec3f>(y,x).val[0] = (b1 + b2) / 2.0;
                result.at<cv::Vec3f>(y,x).val[1] = (g1 + g2) / 2.0;
                result.at<cv::Vec3f>(y,x).val[2] = (r1 + r2) / 2.0;
            }
        }

        break;
    }
    }

    return result;
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

        qDebug() << "target" << i << ":" << width << height;

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

QImage ImageStacker::Mat2QImage(const cv::Mat &src)
{
    QImage dest(src.cols, src.rows, QImage::Format_RGB32);
    int r, g, b;

    if (GetBitsPerChannel() == ImageStacker::BITS_16) {
        for(int x = 0; x < src.cols; x++) {
            for(int y = 0; y < src.rows; y++) {

                cv::Vec<unsigned short,3> pixel = src.at< cv::Vec<unsigned short,3> >(y,x);
                b = pixel.val[0]/256;
                g = pixel.val[1]/256;
                r = pixel.val[2]/256;
                dest.setPixel(x, y, qRgb(r,g,b));
            }
        }
    }
    else if (GetBitsPerChannel() == ImageStacker::BITS_32) {
        for(int x = 0; x < src.cols; x++) {
            for(int y = 0; y < src.rows; y++) {

                cv::Vec3f pixel = src.at<cv::Vec3f>(y,x);
                b = pixel.val[0]*255;
                g = pixel.val[1]*255;
                r = pixel.val[2]*255;
                dest.setPixel(x, y, qRgb(r,g,b));
            }
        }
    }
    return dest;
}

void ImageStacker::StackDarks()
{
    cv::Mat dark1 = ReadImage(dark_frame_file_names_.at(0));
    if (use_bias_) dark1 -= master_bias_;

    cv::Mat result = dark1.clone();

    QString message;

    for (int i = 1; i < dark_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat dark = ReadImage(dark_frame_file_names_.at(i));

        message = tr("Stacking dark frame %1 of %2").arg(QString::number(i+1), QString::number(dark_frame_file_names_.length()));
        qInfo() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_) dark -= master_bias_;
        result += dark;
    }

    result /= dark_frame_file_names_.length();

    master_dark_ = result;
}

void ImageStacker::StackDarkFlats()
{
    cv::Mat darkFlat1 = ReadImage(dark_flat_frame_file_names_.at(0));
    if (use_bias_) darkFlat1 -= master_bias_;

    cv::Mat result = darkFlat1.clone();

    QString message;

    for (int i = 1; i < dark_flat_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat dark = ReadImage(dark_flat_frame_file_names_.at(i));

        message = tr("Stacking dark flat frame %1 of %2").arg(QString::number(i+1), QString::number(dark_flat_frame_file_names_.length()));
        qInfo() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_) dark -= master_bias_;
        result += dark;
    }

    result /= dark_flat_frame_file_names_.length();

    master_dark_flat_ = result;
}

void ImageStacker::StackFlats()
{
    // most algorithms compute the median, but we will stick with mean for now
    cv::Mat flat1 = ReadImage(flat_frame_file_names_.at(0));
    cv::Mat result = flat1.clone();

    if (use_bias_) result -= master_bias_;
    if (use_dark_flats_) result -= master_dark_flat_;

    QString message;

    for (int i = 1; i < flat_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat flat = ReadImage(flat_frame_file_names_.at(i));

        message = tr("Stacking flat frame %1 of %2").arg(QString::number(i+1), QString::number(flat_frame_file_names_.length()));
        qInfo() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        if (use_bias_) flat -= master_bias_;
        if (use_dark_flats_) flat -= master_dark_flat_;
        result += flat;
    }

    result /= flat_frame_file_names_.length();

    // scale the flat frame so that the average value is 1.0
    // since we're dividing, flat values darker than the average value will brighten the image
    //  and values brighter than the average will darken the image, flattening the field
    cv::Scalar meanScalar = cv::mean(result);
    float avg = (meanScalar.val[0] + meanScalar.val[1] + meanScalar.val[2])/3;

    if (bits_per_channel_ == BITS_16)
        result.convertTo(master_flat_, CV_32F, 1.0/avg);
    else if (bits_per_channel_ == BITS_32)
        master_flat_ = result / avg;
}

void ImageStacker::StackBias()
{
    cv::Mat bias1 = ReadImage(bias_frame_file_names_.at(0));
    cv::Mat result = bias1.clone();

    QString message;

    for (int i = 1; i < bias_frame_file_names_.length() && !cancel_; i++) {
        cv::Mat bias = ReadImage(bias_frame_file_names_.at(i));

        message = tr("Stacking bias frame %1 of %2").arg(QString::number(i+1), QString::number(bias_frame_file_names_.length()));
        qInfo() << message;
        current_operation_++;
        if (total_operations_ != 0) emit UpdateProgress(message, 100*current_operation_/total_operations_);

        result += bias;
    }

    result /= bias_frame_file_names_.length();

    master_bias_ = result;
}

cv::Mat ImageStacker::ConvertAndScaleImage(cv::Mat image)
{
    cv::Mat result;

    if (bits_per_channel_ == BITS_16) {
        result = cv::Mat(image.rows, image.cols, CV_16UC3);
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
    else if (bits_per_channel_ == BITS_32) {
        result = cv::Mat(image.rows, image.cols, CV_32FC3);
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
            image.convertTo(result, CV_32F, 1.0/2147483647.0, 1.0);
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

cv::Mat ImageStacker::RawToMat(QString filename)
{
    LibRaw processor;

    // params for raw processing
    processor.imgdata.params.use_auto_wb = 0;
    processor.imgdata.params.use_camera_wb = 1;
    processor.imgdata.params.no_auto_bright = 1;
    processor.imgdata.params.output_bps = 16;

    processor.open_file(filename.toUtf8().constData());
    processor.unpack();

    // process raw into readable BGR bitmap
    processor.dcraw_process();

    libraw_processed_image_t *proc = processor.dcraw_make_mem_image();
    cv::Mat tmp = cv::Mat(cv::Size(processor.imgdata.sizes.width, processor.imgdata.sizes.height),
                    CV_16UC3, proc->data);

    // copy data -- slower, but then we can rely on OpenCV's reference counting
    // also, we have to convert RGB->BGR anyway
    cv::Mat image = cv::Mat(processor.imgdata.sizes.width, processor.imgdata.sizes.height, CV_16UC3);
    cvtColor(tmp, image, CV_RGB2BGR);

    if (bits_per_channel_ == BITS_32)
        image.convertTo(image, CV_32F, 1/65535.0);

    // free the memory that otherwise wouldn't have been handled by OpenCV
    processor.recycle();

    return image;
}

cv::Mat ImageStacker::ReadImage(QString filename)
{
    cv::Mat result;

    QFileInfo info(filename);
    QString ext = info.completeSuffix();

    // assumption: if it looks like a raw file, it is a raw file
    if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
        result = RawToMat(filename);
    }
    else {
        result = cv::imread(filename.toUtf8().constData(), CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        result = ConvertAndScaleImage(result);
    }

    return result;
}

// derived from FOCAS mktransform.c
cv::Mat ImageStacker::GenerateAlignedImage(cv::Mat ref, cv::Mat target, int *ok) {
    StarDetector sd;
    std::vector<Star> List1 = sd.GetStars(ref);
    std::vector<Star> List2 = sd.GetStars(target);

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

ImageStacker::BitsPerChannel ImageStacker::GetBitsPerChannel() const
{
    mutex_.lock();
    BitsPerChannel value = bits_per_channel_;
    mutex_.unlock();

    return value;
}

void ImageStacker::SetBitsPerChannel(const BitsPerChannel &value)
{
    mutex_.lock();
    bits_per_channel_ = value;
    mutex_.unlock();
}

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
