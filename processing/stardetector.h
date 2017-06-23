#ifndef STARDETECTOR_H
#define STARDETECTOR_H

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "model/adjoiningpixel.h"
#include <stack>
#include <QDebug>
#include <string>
#include <string.h>

//! Manages star analysis and detection.
class StarDetector
{
public:
    //! Constructor (currently empty).
    StarDetector();

    //! Destructor (currently empty).
    ~StarDetector();

    //! Analyzes the given image and returns a list of detected stars.
    /*!
        @param image The image containing the stars.
        @return A list of detected stars.
    */
    std::vector<Star> GetStars(cv::Mat image);

    //! Gets the value of the pixel at the given coordinates, truncating to the edges if the coordinates are outside the image bounds.
    /*!
        @param image The image to get the pixel value from.
        @param x The x coordinate of the desired pixel.
        @param y The y coordinate of the desired pixel.
        @return The value of the pixel at (x,y), truncated to the bounds of the image.
    */
    float GetExtendedPixelValue(cv::Mat image, int x, int y);

    //! Generates approximately what the sky background would look like without the stars.
    /*!
        @param image The full image with stars.
        @return The sky background.
    */
    cv::Mat GenerateSkyBackground(cv::Mat image);

    //! Draw a white-on-black plot of the provided stars.
    /*! This started as a debugging tool but ended up being really cool and potentially useful.
        @param path Filename for the output image.
        @param width The width of the resulting image in pixels.
        @param height The height of the resulting image in pixels.
        @param limit Draw only the brightest N stars, or all of the stars if a negative value is given.
        @param stars The list of stars to draw.
    */
    void DrawDetectedStars(const std::string& path, uint width, uint height, int limit, std::vector<Star> stars);

    void test();

private:
    std::vector<AdjoiningPixel> GetAdjoiningPixels(cv::Mat image, float threshold, float minPeak);
    AdjoiningPixel DetectAdjoiningPixel(cv::Mat image, int x, int y, float threshold);
};

#endif // STARDETECTOR_H
