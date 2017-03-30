#include "pixel.h"

Pixel::Pixel()
{

}

Pixel::Pixel(int x_, int y_, float value_)
{
    x = x_;
    y = y_;
    value = value_;
}

int Pixel::getX() const
{
    return x;
}

void Pixel::setX(int value)
{
    x = value;
}

int Pixel::getY() const
{
    return y;
}

void Pixel::setY(int value)
{
    y = value;
}

float Pixel::getValue() const
{
    return value;
}

void Pixel::setValue(float value)
{
    value = value;
}

bool Pixel::operator>(const Pixel &other) const
{
    return value > other.getValue();
}

bool Pixel::operator<(const Pixel &other) const
{
    return value < other.getValue();
}
