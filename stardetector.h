#ifndef STARDETECTOR_H
#define STARDETECTOR_H

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "adjoiningpixel.h"
#include <stack>
#include <QDebug>
#include <string>
#include <string.h>


class StarDetector
{
public:
    StarDetector();
    ~StarDetector();

    void process(cv::Mat image);

    float getExtendedPixelValue(cv::Mat image, int x, int y);
    cv::Mat generateSkyBackground(cv::Mat image);

    void drawDetectedStars(const std::string& path, uint width, uint height, uint limit, std::vector<Star> stars);

    void test();

private:
    std::vector<AdjoiningPixel> getAdjoiningPixels(cv::Mat image, float threshold, float minPeak);
    AdjoiningPixel detectAdjoiningPixel(cv::Mat image, int x, int y, float threshold);
};

#endif // STARDETECTOR_H
