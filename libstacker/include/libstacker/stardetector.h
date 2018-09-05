#ifndef STARDETECTOR_H
#define STARDETECTOR_H

#include "libstacker/libstacker_global.h"
#include "libstacker/model.h"

#include "opencv2/core/core.hpp"

#include <memory>

namespace openskystacker {

//! Manages star analysis and detection.
class LIBSTACKER_EXPORT StarDetector
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
    std::vector<Star> getStars(cv::Mat image, int thresholdCoeff);
    std::vector<Star> getStars(cv::Mat image);

    //! Generates approximately what the sky background would look like without the stars.
    /*!
        @param image The full image with stars.
        @return The sky background.
    */
    cv::Mat generateSkyBackground(cv::Mat image);

    using size_type = std::vector<Star>::size_type;
    //! Draw a white-on-black plot of the provided stars.
    /*! This started as a debugging tool but ended up being really cool and potentially useful.
        @param path Filename for the output image.
        @param width The width of the resulting image in pixels.
        @param height The height of the resulting image in pixels.
        @param limit Draw only the brightest N stars, or all of the stars if a negative value is given.
        @param stars The list of stars to draw.
    */
    void drawDetectedStars(const std::string& path, uint width, uint height, size_type limit, std::vector<Star> stars);

private:
    class StarDetectorImpl;
    std::unique_ptr<StarDetectorImpl> dPtr;
};

}

#endif // STARDETECTOR_H
