#ifndef OSS_MODEL_H
#define OSS_MODEL_H

#include "libstacker/libstacker_global.h"

#include <QString>
#include <ctime>

namespace openskystacker {

//! Contains metadata regarding an image.
struct ImageRecord
{
    //! Describes the type of frame (e.g. Light, Dark, etc.).
    enum FrameType {
        LIGHT,     /*!< Light frame. */
        DARK,      /*!< Dark frame. */
        DARK_FLAT, /*!< Dark flat frame. */
        FLAT,      /*!< Flat frame. */
        BIAS       /*!< Bias/offset frame. */
    };

    QString filename;
    FrameType type;
    float shutter;
    float iso;
    time_t timestamp;
    bool reference;
    bool checked;
    int width;
    int height;
};

struct Star
{
    Star() {}
    Star(int x, int y, float value) : x(x), y(y), value(value) {}

    bool operator==(const Star& s) const
    {
        return x == s.x && y == s.y;
    }

    bool operator>(const Star& other) const
    {
        return value > other.value;
    }

    bool operator<(const Star& other) const
    {
        return value < other.value;
    }

    int x;
    int y;
    int area;

    float peak;
    float radius;
    float value;
};

struct Pixel
{
    Pixel() {}

    //! Constructor.
    /*! @param x The x coordinate of the pixel.
        @param y The y coordinate of the pixel.
        @param value The intensity of the pixel.
    */
    Pixel(int x, int y, float value) : x(x), y(y), value(value) {}

    bool operator>(const Pixel& other) const
    {
        return value > other.value;
    }
    bool operator<(const Pixel& other) const
    {
        return value < other.value;
    }

    int x;
    int y;
    float value;
};

struct Triangle
{
    //! Constructor.
    Triangle() {}

    //! Constructor.
    /*! @param s1_ The index of the Star between sides A and B.
        @param s2_ The index of the Star between sides A and C.
        @param s3_ The index of the Star between sides B and C.
    */
    Triangle(int s1, int s2, int s3) : s1(s1), s2(s2), s3(s3) {}

    // indices of corresponding objects
    //! The index of the Star between sides A and B.
    int s1;

    //! The index of the Star between sides A and C.
    int s2;

    //! The index of the Star between sides B and C.
    int s3;

    // coordinates in triangle space
    //! The x coordinate in triangle space.
    float x;
    //! The y coordinate in triangle space
    float y;
};

}

#endif // OSS_MODEL_H
