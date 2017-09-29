#ifndef ADJOININGPIXEL_H
#define ADJOININGPIXEL_H

#include "pixel.h"
#include "star.h"

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
    float GetPeakValue();

    //! Gets the peak pixel.
    /*! @return The peak Pixel. */
    Pixel GetPeak();

    //! Adds a pixel to the list of pixels.
    /*! @param pixel The Pixel to add to the list. */
    void AddPixel(Pixel pixel);

    //! Separates this AdjoiningPixel into potentially multiple AdjoiningPixels
    /*! @param base_step The step to deblend.
        @return A list of deblended AdjoiningPixels.
    */
    std::vector<AdjoiningPixel> Deblend(float base_step);

    //! Gets the weighted center of the AdjoiningPixel.
    /*! @return A point containing the coordinates of the weighted center of the AdjoiningPixel. */
    cv::Point GetGravityCenter();

    //! Constructs a Star from this AdjoiningPixel.
    /*! @return A star based on this AdjoiningPixel. */
    Star CreateStar();

    bool operator>(const AdjoiningPixel& other) const;
    bool operator<(const AdjoiningPixel& other) const;

    //! Gets the list of Pixels pertaining to this AdjoiningPixel.
    /*! @return The list of Pixels. */
    std::vector<Pixel> GetPixels() const;

    //! Sets the list of Pixels pertaining to this AdjoingingPixel.
    /*! @param value The list of Pixels. */
    void SetPixels(const std::vector<Pixel> &value);

private:
    void Extract(int x, int y);
    bool IsAdjoining(AdjoiningPixel ap);

    AdjoiningPixel *current_ap_;
    float current_threshold_;
    std::vector<Pixel> pixels_;
};

}

#endif // ADJOININGPIXEL_H
