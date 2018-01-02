#include "star.h"

using namespace openskystacker;

Star::Star()
{

}

Star::Star(int x, int y, float value)
{
    x_ = x;
    y_ = y;
    value_ = value;
}

Star::~Star()
{

}

bool Star::operator==(const Star &s)
{
    return x_ == s.GetX() && y_ == s.GetY();
}

bool Star::operator>(const Star &other) const
{
    return value_ > other.GetValue();
}

bool Star::operator<(const Star &other) const
{
    return value_ < other.GetValue();
}

int Star::GetX() const
{
    return x_;
}

void Star::SetX(int value)
{
    x_ = value;
}

int Star::GetY() const
{
    return y_;
}

void Star::SetY(int value)
{
    y_ = value;
}

float Star::GetPeak() const
{
    return peak_;
}

void Star::SetPeak(float value)
{
    peak_ = value;
}

float Star::GetRadius() const
{
    return radius_;
}

void Star::SetRadius(float value)
{
    radius_ = value;
}

int Star::GetArea() const
{
    return area_;
}

void Star::SetArea(int value)
{
    area_ = value;
}

float Star::GetValue() const
{
    return value_;
}

void Star::SetValue(float value)
{
    value_ = value;
}
