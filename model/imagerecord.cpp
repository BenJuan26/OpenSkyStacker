#include "model/imagerecord.h"

ImageRecord::ImageRecord()
{

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
