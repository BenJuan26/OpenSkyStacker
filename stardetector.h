#ifndef STARDETECTOR_H
#define STARDETECTOR_H

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "adjoiningpixel.h"
#include <stack>
#include <QDebug>


class StarDetector
{
public:
    StarDetector();
    ~StarDetector();

    void process(cv::Mat image);

    float getExtendedPixelValue(cv::Mat image, int x, int y);
    cv::Mat generateSkyBackground(cv::Mat image);

private:
    std::vector<AdjoiningPixel> getAdjoiningPixels(cv::Mat image, float threshold, float minPeak);
    AdjoiningPixel detectAdjoiningPixel(cv::Mat image, int x, int y, float threshold);
};

#endif // STARDETECTOR_H
