#include "model/imagerecord.h"

ImageRecord::ImageRecord()
{
    reference_ = false;
    checked_ = true;
}

QString ImageRecord::GetFilename() const
{
    return filename_;
}

void ImageRecord::SetFilename(const QString &value)
{
    filename_ = value;
}

ImageRecord::FrameType ImageRecord::GetType() const
{
    return type_;
}

void ImageRecord::SetType(const FrameType &value)
{
    type_ = value;
}

float ImageRecord::GetShutter() const
{
    return shutter_;
}

void ImageRecord::SetShutter(float value)
{
    shutter_ = value;
}

float ImageRecord::GetIso() const
{
    return iso_;
}

void ImageRecord::SetIso(float value)
{
    iso_ = value;
}

bool ImageRecord::IsReference() const
{
    return reference_;
}

void ImageRecord::SetReference(bool value)
{
    reference_ = value;
}

time_t ImageRecord::GetTimestamp() const
{
    return timestamp_;
}

void ImageRecord::SetTimestamp(const time_t &value)
{
    timestamp_ = value;
}

bool ImageRecord::IsChecked() const
{
    return checked_;
}

void ImageRecord::SetChecked(bool value)
{
    checked_ = value;
}

int ImageRecord::GetWidth() const
{
    return width_;
}

void ImageRecord::SetWidth(int value)
{
    width_ = value;
}

int ImageRecord::GetHeight() const
{
    return height_;
}

void ImageRecord::SetHeight(int value)
{
    height_ = value;
}
