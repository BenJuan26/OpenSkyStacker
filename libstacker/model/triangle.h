#ifndef TRIANGLE_H
#define TRIANGLE_H

namespace openskystacker {

//! Represents a triangle in the context of image alignment using the FOCAS algorithm.
class Triangle
{
public:
    //! Constructor.
    Triangle();

    //! Constructor.
    /*! @param s1_ The index of the Star between sides A and B.
        @param s2_ The index of the Star between sides A and C.
        @param s3_ The index of the Star between sides B and C.
    */
    Triangle(int s1, int s2, int s3);

    // indices of corresponding objects
    //! The index of the Star between sides A and B.
    int s1_;

    //! The index of the Star between sides A and C.
    int s2_;

    //! The index of the Star between sides B and C.
    int s3_;

    // coordinates in triangle space
    //! The x coordinate in triangle space.
    float x_;
    //! The y coordinate in triangle space
    float y_;
};

}

#endif // TRIANGLE_H
