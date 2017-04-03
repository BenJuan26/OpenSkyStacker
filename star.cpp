#include "star.h"

Star::Star()
{

}

Star::Star(int x_, int y_, float value_)
{
    x = x_;
    y = y_;
    value = value_;
}

Star::~Star()
{

}

bool Star::operator==(const Star &s)
{
    return x == s.getX() && y == s.getY();
}

bool Star::operator>(const Star &other) const
{
    return value > other.getValue();
}

bool Star::operator<(const Star &other) const
{
    return value < other.getValue();
}

int Star::getX() const
{
    return x;
}

void Star::setX(int value)
{
    x = value;
}

int Star::getY() const
{
    return y;
}

void Star::setY(int value)
{
    y = value;
}

float Star::getPeak() const
{
    return peak;
}

void Star::setPeak(float value)
{
    peak = value;
}

float Star::getRadius() const
{
    return radius;
}

void Star::setRadius(float value)
{
    radius = value;
}

int Star::getArea() const
{
    return area;
}

void Star::setArea(int value)
{
    area = value;
}

float Star::getValue() const
{
    return value;
}

void Star::setValue(float newValue)
{
    value = newValue;
}
