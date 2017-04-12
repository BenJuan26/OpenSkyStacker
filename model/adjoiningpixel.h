#ifndef ADJOININGPIXEL_H
#define ADJOININGPIXEL_H

#include "pixel.h"
#include "star.h"
#include <vector>
#include <math.h>
#include <stdlib.h>
#include "opencv2/core/types.hpp"

typedef unsigned long ulong;


class AdjoiningPixel
{
public:
    AdjoiningPixel();
    ~AdjoiningPixel();
    float getPeakValue();
    Pixel getPeak();
    void addPixel(Pixel pixel);

    std::vector<AdjoiningPixel> deblend(float base_step);
    cv::Point getGravityCenter();
    Star createStar();

    bool operator>(const AdjoiningPixel& other) const;
    bool operator<(const AdjoiningPixel& other) const;

    std::vector<Pixel> getPixels() const;
    void setPixels(const std::vector<Pixel> &value);

private:
    std::vector<Pixel> pixels;
    void extract(int x, int y);
    float currentThreshold;
    bool isAdjoining(AdjoiningPixel ap);

    AdjoiningPixel *currentAp;
};

#endif // ADJOININGPIXEL_H
