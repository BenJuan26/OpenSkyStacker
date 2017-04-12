#ifndef PIXEL_H
#define PIXEL_H


class Pixel
{
public:
    Pixel();
    Pixel(int x, int y, float value);

    int getX() const;
    void setX(int value);

    int getY() const;
    void setY(int value);

    float getValue() const;
    void setValue(float otherValue);

    bool operator>(const Pixel& other) const;
    bool operator<(const Pixel& other) const;

private:
    int x;
    int y;
    float value;
};

#endif // PIXEL_H
