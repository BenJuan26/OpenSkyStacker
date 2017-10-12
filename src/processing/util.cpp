#include "processing/util.h"

using namespace openskystacker;
using namespace CCfits;
using namespace easyexif;

cv::Mat openskystacker::GetBayerMatrix(QString filename) {
    LibRaw libraw;
    libraw_data_t *imgdata = &libraw.imgdata;
    libraw_output_params_t *params = &imgdata->params;

    // params for raw processing
    params->use_auto_wb = 0;
    params->use_camera_wb = 1;
    params->no_auto_bright = 1;
    params->output_bps = 16;

    libraw.open_file(filename.toUtf8().constData());
    libraw.unpack();

    cv::Mat bayer(imgdata->sizes.raw_width, imgdata->sizes.raw_height,
            CV_16UC1, imgdata->rawdata.raw_image);
    return bayer.clone();
}

ImageRecord *openskystacker::GetImageRecord(QString filename)
{
    ImageRecord *record = new ImageRecord();
    record->SetFilename(filename);

    switch (GetImageType(filename)) {
    case RAW_IMAGE: {
        LibRaw processor;

        processor.open_file(filename.toUtf8().constData());

        libraw_imgother_t other = processor.imgdata.other;

        record->SetIso(other.iso_speed);
        record->SetShutter(other.shutter);
        record->SetTimestamp(other.timestamp);
        record->SetWidth(processor.imgdata.sizes.width);
        record->SetHeight(processor.imgdata.sizes.height);
        break;
    }
    case FITS_IMAGE: {
        std::auto_ptr<FITS> pInFile(new FITS(filename.toUtf8().constData(), Read));
        PHDU &image = pInFile->pHDU();
        image.readAllKeys();

        int exp;
        try {
            image.readKey<int>("EXPTIME", exp);
        } catch (HDU::NoSuchKeyword) {
            try {
                image.readKey<int>("EXPOSURE", exp);
            } catch (HDU::NoSuchKeyword) {
                exp = -1;
            }
        }

        std::string date;
        try {
            image.readKey<std::string>("DATE-OBS", date);
        } catch (HDU::NoSuchKeyword) {
            date = "";
        }

        record->SetIso(-1);
        record->SetShutter(exp);
        record->SetTimestamp(FITSTimeToCTime(date));
        record->SetWidth(image.axis(0));
        record->SetHeight(image.axis(1));

        break;
    }
    case RGB_IMAGE: default: {
        QFile file(filename);
        file.open(QIODevice::ReadOnly);
        QByteArray blob = file.readAll();

        EXIFInfo exif;
        if (!exif.parseFrom((unsigned char*)blob.constData(), blob.size())) {
            record->SetIso(exif.ISOSpeedRatings);
            record->SetShutter(exif.ExposureTime);
            record->SetTimestamp(EXIFTimeToCTime(exif.DateTime));
            record->SetWidth(exif.ImageWidth);
            record->SetHeight(exif.ImageHeight);
        } else {
            record->SetIso(-1);
            record->SetShutter(-1);
            record->SetTimestamp(-1);

            cv::Mat image = cv::imread(filename.toUtf8().constData());
            if (image.data) {
                record->SetWidth(image.cols);
                record->SetHeight(image.rows);
            } else {
                record->SetWidth(-1);
                record->SetHeight(-1);
            }
        }
        break;
    }
    }

    return record;
}

time_t openskystacker::EXIFTimeToCTime(std::string exifTime)
{
    // EXIF format: YYYY:MM:DD HH:MM:SS

    QString timeString(exifTime.c_str());
    QStringList dateAndTime = timeString.split(' ');
    QString imageDate = dateAndTime.at(0);
    QString imageTime = dateAndTime.at(1);

    QStringList dateList = imageDate.split(':');
    QString year = dateList.at(0);
    QString mon = dateList.at(1);
    QString mday = dateList.at(2);

    QStringList timeList = imageTime.split(':');
    QString hour = timeList.at(0);
    QString min = timeList.at(1);
    QString sec = timeList.at(2);

    struct tm tm;
    tm.tm_year = year.toInt() - 1900; // years since 1900
    tm.tm_mon = mon.toInt() - 1;      // month is 0-indexed
    tm.tm_mday = mday.toInt();
    tm.tm_hour = hour.toInt();
    tm.tm_min = min.toInt();
    tm.tm_sec = sec.toInt();
    tm.tm_isdst = -1;                 // tell the library to use local DST

    return mktime(&tm);
}

time_t openskystacker::FITSTimeToCTime(std::string fitsTime)
{
    // ISO 8601 format: yyyy-mm-ddTHH:MM:SS[.sss]

    QString timeString(fitsTime.c_str());
    QStringList dateAndTime = timeString.split('T');
    QString imageDate = dateAndTime.at(0);
    QString imageTime = dateAndTime.at(1);

    QStringList dateList = imageDate.split('-');
    QString year = dateList.at(0);
    QString mon = dateList.at(1);
    QString mday = dateList.at(2);

    QStringList timeList = imageTime.split(':');
    QString hour = timeList.at(0);
    QString min = timeList.at(1);
    QString sec = timeList.at(2);

    struct tm tm;
    tm.tm_year = year.toInt() - 1900; // years since 1900
    tm.tm_mon = mon.toInt() - 1;      // month is 0-indexed
    tm.tm_mday = mday.toInt();
    tm.tm_hour = hour.toInt();
    tm.tm_min = min.toInt();
    tm.tm_sec = sec.toInt();
    tm.tm_isdst = -1;                 // tell the library to use local DST

    return mktime(&tm);
}

openskystacker::ImageType openskystacker::GetImageType(QString filename)
{
    QFileInfo info(filename);
    QString ext = info.suffix();

    if (std::find(FITS_EXTENSIONS.begin(), FITS_EXTENSIONS.end(), ext.toLower()) != FITS_EXTENSIONS.end()) {
        return FITS_IMAGE;
    } else if (std::find(RAW_EXTENSIONS.begin(), RAW_EXTENSIONS.end(), ext.toLower()) != RAW_EXTENSIONS.end()) {
        return RAW_IMAGE;
    } else {
        return RGB_IMAGE;
    }
}

cv::Mat openskystacker::GetCalibratedImage(QString filename, cv::Mat dark, cv::Mat flat, cv::Mat bias) {
    if (GetImageType(filename) == RAW_IMAGE) {
        LibRaw libraw;
        libraw_data_t *imgdata = &libraw.imgdata;
        libraw_output_params_t *params = &imgdata->params;

        // params for raw processing
        params->use_auto_wb = 0;
        params->use_camera_wb = 1;
        params->no_auto_bright = 1;
        params->output_bps = 16;

        libraw.open_file(filename.toUtf8().constData());
        libraw.unpack();

        // Edit the LibRaw Bayer matrix directly with a Mat
        // That way we can still use LibRaw's debayering and processing algorithms
        cv::Mat bayer(imgdata->sizes.raw_width, imgdata->sizes.raw_height,
                CV_16UC1, imgdata->rawdata.raw_image);

        if (bias.dims > 0)  bayer -= bias;
        if (dark.dims > 0) bayer -= dark;
        if (flat.dims > 0) cv::divide(bayer, flat, bayer, 1.0, CV_16U);

        // process raw into readable BGR bitmap
        libraw.dcraw_process();

        libraw_processed_image_t *proc = libraw.dcraw_make_mem_image();
        cv::Mat image = cv::Mat(cv::Size(imgdata->sizes.width, imgdata->sizes.height),
                        CV_16UC3, proc->data);
        cv::Mat result = image.clone();

        delete proc;

        result.convertTo(result, CV_32F, 1/65535.0);

        return result;
    } else {
        cv::Mat result = ReadImage(filename);

        if (bias.dims > 0)  cv::subtract(result, bias, result, cv::noArray(), CV_32F);
        if (dark.dims > 0) cv::subtract(result, dark, result, cv::noArray(), CV_32F);
        if (flat.dims > 0) cv::divide(result, flat, result, 1.0, CV_32F);

        return result;
    }
}

cv::Mat openskystacker::ReadImage(QString filename)
{
    cv::Mat result;

    openskystacker::ImageType type = GetImageType(filename);

    // assumption: if it looks like a raw file, it is a raw file
    switch(type) {
    case RAW_IMAGE:
        result = RawToMat(filename);
        break;
    case FITS_IMAGE:
        result = FITSToMat(filename);
        break;
    case RGB_IMAGE: default:
        result = cv::imread(filename.toUtf8().constData(), CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        result = ConvertAndScaleImage(result);
        break;
    }

    return result;
}

cv::Mat openskystacker::FITSToMat(QString filename)
{
    std::auto_ptr<FITS> pInFile(new FITS(filename.toUtf8().constData(), Read, true));
    PHDU &image = pInFile->pHDU();
    image.readAllKeys();

    cv::Mat result;
    long rows = image.axis(1);
    long cols = image.axis(0);

    long bits = image.bitpix();
    switch(bits) {
    case 8: {
        std::valarray<unsigned char> contents;
        image.read(contents);
        cv::Mat tmp = cv::Mat(rows, cols, CV_8UC1, &contents[0]);
        tmp.convertTo(result, CV_32F, 1.0/255.0);
        break;
    }
    case 16: default: {
        std::valarray<unsigned short> contents;
        image.read(contents);
        cv::Mat tmp = cv::Mat(rows, cols, CV_16UC1, &contents[0]);
        tmp.convertTo(result, CV_32F, 1.0/65535.0);
        break;
    }
    case 32: {
        std::valarray<unsigned long> contents;
        image.read(contents);
        cv::Mat tmp = cv::Mat(rows, cols, CV_32SC1, &contents[0]);
        tmp.convertTo(result, CV_32F, 1.0/2147483647.0, 2147483648);
        break;
    }
    case -32: {
        std::valarray<float> contents;
        image.read(contents);
        cv::Mat tmp = cv::Mat(rows, cols, CV_32FC1, &contents[0]);
        result = tmp.clone();
        break;
    }
    case -64: {
        std::valarray<double> contents;
        image.read(contents);
        cv::Mat tmp = cv::Mat(rows, cols, CV_64FC1, &contents[0]);
        tmp.convertTo(result, CV_32F);
        break;
    }
    }

    return result;
}

cv::Mat openskystacker::RawToMat(QString filename)
{
    LibRaw processor;
    libraw_data_t *imgdata = &processor.imgdata;
    libraw_output_params_t *params = &imgdata->params;

    // params for raw processing
    params->use_auto_wb = 0;
    params->use_camera_wb = 1;
    params->no_auto_bright = 1;
    params->output_bps = 16;

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

    delete proc;

    image.convertTo(image, CV_32F, 1/65535.0);

    return image;
}

cv::Mat openskystacker::ConvertAndScaleImage(cv::Mat image)
{
    cv::Mat result;

    if (image.channels() == 1)
        result = cv::Mat(image.rows, image.cols, CV_32FC1);
    else
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

    return result;
}

std::vector<ImageRecord *> openskystacker::LoadImageList(QString filename, int *err)
{
    std::vector<ImageRecord *> result;

    QFile file(filename);
    QString contents;
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        contents = in.readAll();
    }

    QFileInfo info(file);
    QString absolutePathToJson = info.absolutePath();

    QJsonDocument doc = QJsonDocument::fromJson(contents.toUtf8());
    if (!doc.isArray()) {
        if (err)
            *err = -1;
        return result;
    }

    QJsonArray list = doc.array();
    for (int i = 0; i < list.size(); i++) {
        QJsonValue val = list.at(i);
        QJsonObject img = val.toObject();
        if (img.isEmpty()) {
            if (err)
                *err = -2;
            return result;
        }

        QString imageFileName = img.value("filename").toString();
        if (imageFileName.isNull()) {
            if (err)
                *err = -3;
            return result;
        }
        QFileInfo imageInfo(imageFileName);
        if (imageInfo.isRelative())
            imageFileName = absolutePathToJson + "/" + imageFileName;

        int type = img.value("type").toInt(-1);
        if (type < 0) {
            if (err)
                *err = -4;
            return result;
        }

        bool checked = img.value("checked").toBool();
        ImageRecord *record = GetImageRecord(imageFileName);
        record->SetType(static_cast<ImageRecord::FrameType>(type));
        record->SetChecked(checked);

        result.push_back(record);
    }

    return result;
}

QImage openskystacker::Mat2QImage(const cv::Mat &src)
{
    QImage dest(src.cols, src.rows, QImage::Format_RGB32);
    int r, g, b;

    for(int x = 0; x < src.cols; x++) {
        for(int y = 0; y < src.rows; y++) {
            if (src.channels() == 1) {
                float pixel = src.at<float>(y, x);
                int value = pixel * 255;
                dest.setPixel(x,y,qRgb(value, value, value));
            } else {
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
