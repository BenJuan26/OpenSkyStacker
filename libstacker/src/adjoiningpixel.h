#ifndef ADJOININGPIXEL_H
#define ADJOININGPIXEL_H

#include "libstacker/model.h"

#include <vector>
#include <math.h>
#include <stdlib.h>

#include "opencv2/opencv.hpp"

typedef unsigned long ulong;

namespace openskystacker {

//! A group of pixels potentially representing one or more Stars.
/*! Derived from Java code from PIXY2 (https://github.com/jankotek/Pixy2).
*/
class AdjoiningPixel
{
public:
    AdjoiningPixel();
    ~AdjoiningPixel();

    //! Gets the value of the peak pixel.
    /*! @return The peak value. */
    float getPeakValue();

    //! Gets the peak pixel.
    /*! @return The peak Pixel. */
    Pixel getPeak();

    //! Adds a pixel to the list of pixels.
    /*! @param pixel The Pixel to add to the list. */
    void addPixel(Pixel pixel);

    //! Separates this AdjoiningPixel into potentially multiple AdjoiningPixels
    /*! @param base_step The step to deblend.
        @return A list of deblended AdjoiningPixels.
    */
    std::vector<AdjoiningPixel> deblend(float base_step);

    //! Gets the weighted center of the AdjoiningPixel.
    /*! @return A point containing the coordinates of the weighted center of the AdjoiningPixel. */
    cv::Point getGravityCenter();

    //! Constructs a Star from this AdjoiningPixel.
    /*! @return A star based on this AdjoiningPixel. */
    Star createStar();

    bool operator>(const AdjoiningPixel& other) const;
    bool operator<(const AdjoiningPixel& other) const;

    //! Gets the list of Pixels pertaining to this AdjoiningPixel.
    /*! @return The list of Pixels. */
    std::vector<Pixel> getPixels() const;

    //! Sets the list of Pixels pertaining to this AdjoingingPixel.
    /*! @param value The list of Pixels. */
    void setPixels(const std::vector<Pixel> &value);

private:
    void extract(int x, int y);
    bool isAdjoining(AdjoiningPixel ap);

    AdjoiningPixel *currentAp;
    float currentThreshold;
    std::vector<Pixel> pixels;
};

}

#endif // ADJOININGPIXEL_H
