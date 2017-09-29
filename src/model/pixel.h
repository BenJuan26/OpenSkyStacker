#ifndef PIXEL_H
#define PIXEL_H

namespace openskystacker {

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

    int GetX() const;
    void SetX(int value_);

    int GetY() const;
    void SetY(int value_);

    float GetValue() const;
    void SetValue(float value);

    bool operator>(const Pixel& other) const;
    bool operator<(const Pixel& other) const;

private:
    int x_;
    int y_;
    float value_;
};

}

#endif // PIXEL_H
