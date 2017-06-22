#ifndef PIXEL_H
#define PIXEL_H

//! Represents a pixel in an image.
class Pixel
{
public:
    //! Constructor.
    Pixel();

    //! Constructor.
    /*! @param x The x coordinate of the pixel.
        @param y The y coordinate of the pixel.
        @param value The intensity of the pixel.
    */
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
