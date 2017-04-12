#ifndef TRIANGLE_H
#define TRIANGLE_H


class Triangle
{
public:
    Triangle();
    Triangle(int s1_, int s2_, int s3_);

    // indices of corresponding objects
    int s1; // between a and b
    int s2; // between a and c
    int s3; // between b and c

    // coordinates in triangle space
    float x;
    float y;
};

#endif // TRIANGLE_H
