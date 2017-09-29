#ifndef STAR_H
#define STAR_H

namespace openskystacker {

//! Represents a star as it appears in an image.
class Star
{
public:
    //! Constructor.
    Star();

    //! Constructor.
    /*! @param x_ The x coordinate of the Star.
        @param y_ The y coordinate of the Star.
        @param value_ The intensity of the Star.
    */
    Star(int x, int y, float value);

    //! Destructor.
    ~Star();

    bool operator==(const Star& s);
    bool operator>(const Star& other) const;
    bool operator<(const Star& other) const;

    int GetX() const;
    void SetX(int value_);

    int GetY() const;
    void SetY(int value_);

    float GetPeak() const;
    void SetPeak(float value_);

    float GetRadius() const;
    void SetRadius(float value_);

    int GetArea() const;
    void SetArea(int value_);

    float GetValue() const;
    void SetValue(float value);

private:
    int x_;
    int y_;
    int area_;

    float peak_;
    float radius_;
    float value_;
};

}

#endif // STAR_H
