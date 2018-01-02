#include "pixel.h"

using namespace openskystacker;

Pixel::Pixel()
{

}

Pixel::Pixel(int x, int y, float value)
{
    x_ = x;
    y_ = y;
    value_ = value;
}

int Pixel::GetX() const
{
    return x_;
}

void Pixel::SetX(int value)
{
    x_ = value;
}

int Pixel::GetY() const
{
    return y_;
}

void Pixel::SetY(int value)
{
    y_ = value;
}

float Pixel::GetValue() const
{
    return value_;
}

void Pixel::SetValue(float value)
{
    value_ = value;
}

bool Pixel::operator>(const Pixel &other) const
{
    return value_ > other.GetValue();
}

bool Pixel::operator<(const Pixel &other) const
{
    return value_ < other.GetValue();
}
