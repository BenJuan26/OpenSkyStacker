#include "adjoiningpixel.h"

using namespace openskystacker;

AdjoiningPixel::AdjoiningPixel()
{

}

AdjoiningPixel::~AdjoiningPixel()
{
    //delete currentAp;
}

float AdjoiningPixel::getPeakValue()
{
    return getPeak().value;
}

Pixel AdjoiningPixel::getPeak()
{
    Pixel pixel;
    float max = 0.0;

    for (ulong i = 0; i < pixels.size(); i++) {
        Pixel p = pixels.at(i);
        if (i == 0  || max < p.value) {
            pixel = p;
            max = p.value;
        }
    }

    return pixel;
}

void AdjoiningPixel::addPixel(Pixel pixel)
{
    pixels.push_back(pixel);
}

std::vector<AdjoiningPixel> AdjoiningPixel::deblend(float base_step)
{
    std::vector<AdjoiningPixel> apList;

    if (pixels.size() >= 10000) {
        apList.push_back(*this);
        return apList;
    }

    double radius = 1.0;

    while (pixels.size() > 0) {
        Pixel peakPixel = getPeak();

        // Extracts only pixels surrounding around the peak and
        // creates a new set of adjoining pixels.
        currentThreshold = peakPixel.value - base_step * sqrt(peakPixel.value / base_step + 1.0);

        currentAp = new AdjoiningPixel();
        extract(peakPixel.x, peakPixel.y);
        cv::Point currentCenter = currentAp->getGravityCenter();

        // Determines if the current set of adjoining pixels is
        // a new peak or foot of already detected peak.
        AdjoiningPixel blendingAp;
        int blendingCount = 0;
        for (ulong i = 0 ; i < apList.size() ; i++) {
            AdjoiningPixel ap = apList.at(i);
            if (currentAp->isAdjoining(ap)) {
                blendingAp = ap;
                blendingCount++;
            } else {
                // When the distance between the two peaks is smaller than
                // the mean radius, they must not be separated.
                cv::Point center = ap.getGravityCenter();
                float dist = sqrt((currentCenter.x - center.x) * (currentCenter.x - center.x) + (currentCenter.y - center.y) * (currentCenter.y - center.y));
                if (dist < radius) {
                    blendingAp = ap;
                    blendingCount++;
                }
            }
        }

        if (blendingCount == 0) {
            // New peak.
            apList.push_back(*currentAp);
        } else if (blendingCount == 1) {
            // Foot of an already detected peak.
            for (ulong i = 0 ; i < currentAp->getPixels().size() ; i++) {
                Pixel pixel = currentAp->getPixels().at(i);
                blendingAp.addPixel(pixel);
            }
        } else {
            // Foot of already detected several peaks.
            // In this case, now the pixels are ignored.
        }
    }

    delete currentAp;

    return apList;
}

cv::Point AdjoiningPixel::getGravityCenter()
{
    float x = 0.0;
    float y = 0.0;
    float w = 0.0;

    for (ulong i = 0 ; i < pixels.size() ; i++) {
        Pixel p = pixels.at(i);
        x += p.value * (float)p.x;
        y += p.value * (float)p.y;
        w += p.value;
    }

    return cv::Point(x / w, y / w);
}

Star AdjoiningPixel::createStar()
{
    Star star;
    star.area = static_cast<int>(pixels.size());

    star.peak = getPeakValue();
    float starValue = 0.0;
    for (ulong i = 0; i < pixels.size(); i++) {
        starValue += pixels.at(i).value;
    }
    star.value = starValue;

    std::vector<Pixel> sortedPixels(pixels);
    std::sort(sortedPixels.begin(), sortedPixels.end());

    float xAmount = 0.0;
    float yAmount = 0.0;
    float weight = 0.0;

    for (ulong i = 0; i < pixels.size(); i++) {
        Pixel pixel = sortedPixels.at(i);
        xAmount += (pixel.x + 0.5) * pixel.value;
        yAmount += (pixel.y + 0.5) * pixel.value;
        weight += pixel.value;
    }

    star.x = xAmount / weight;
    star.y = yAmount / weight;

    return star;
}

bool AdjoiningPixel::operator>(const AdjoiningPixel &other) const
{
    return pixels.size() > other.getPixels().size();
}

bool AdjoiningPixel::operator<(const AdjoiningPixel &other) const
{
    return pixels.size() < other.getPixels().size();
}

std::vector<Pixel> AdjoiningPixel::getPixels() const
{
    return pixels;
}

void AdjoiningPixel::setPixels(const std::vector<Pixel> &value)
{
    pixels = value;
}

void AdjoiningPixel::extract(int x, int y)
{
    for (ulong i = 0 ; i < pixels.size() ; i++) {
        Pixel pixel = pixels.at(i);

        if (pixel.x == x  &&  pixel.y == y  &&  pixel.value > currentThreshold) {
            currentAp->addPixel(pixel);
            pixels.erase(pixels.begin() + i);

            extract(x, y-1);
            extract(x-1, y);
            extract(x+1, y);
            extract(x, y+1);

            return;
        }
    }
}

bool AdjoiningPixel::isAdjoining(AdjoiningPixel ap)
{
    for (ulong i = 0 ; i < pixels.size() ; i++) {
        Pixel p1 = pixels.at(i);
        for (ulong j = 0 ; j < ap.getPixels().size() ; j++) {
            Pixel p2 = ap.getPixels().at(j);
            if (abs(p1.x - p2.x) + abs(p1.y - p2.y) <= 1)
                return true;
        }
    }
    return false;
}
