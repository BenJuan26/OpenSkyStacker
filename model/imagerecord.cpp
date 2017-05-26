#include "model/imagerecord.h"

ImageRecord::ImageRecord()
{
    reference = false;
    checked = true;
}

QString ImageRecord::getFilename() const
{
    return filename;
}

void ImageRecord::setFilename(const QString &value)
{
    filename = value;
}

ImageRecord::FRAME_TYPE ImageRecord::getType() const
{
    return type;
}

void ImageRecord::setType(const FRAME_TYPE &value)
{
    type = value;
}

float ImageRecord::getShutter() const
{
    return shutter;
}

void ImageRecord::setShutter(float value)
{
    shutter = value;
}

float ImageRecord::getIso() const
{
    return iso;
}

void ImageRecord::setIso(float value)
{
    iso = value;
}

bool ImageRecord::isReference() const
{
    return reference;
}

void ImageRecord::setReference(bool value)
{
    reference = value;
}

time_t ImageRecord::getTimestamp() const
{
    return timestamp;
}

void ImageRecord::setTimestamp(const time_t &value)
{
    timestamp = value;
}

bool ImageRecord::isChecked() const
{
    return checked;
}

void ImageRecord::setChecked(bool value)
{
    checked = value;
}

int ImageRecord::getWidth() const
{
    return width;
}

void ImageRecord::setWidth(int value)
{
    width = value;
}

int ImageRecord::getHeight() const
{
    return height;
}

void ImageRecord::setHeight(int value)
{
    height = value;
}
