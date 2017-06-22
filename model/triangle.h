#ifndef TRIANGLE_H
#define TRIANGLE_H

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
    Triangle(int s1_, int s2_, int s3_);

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

#endif // TRIANGLE_H
