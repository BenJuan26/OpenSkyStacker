#include "adjoiningpixel.h"

AdjoiningPixel::AdjoiningPixel()
{

}

AdjoiningPixel::~AdjoiningPixel()
{
    //delete currentAp;
}

float AdjoiningPixel::GetPeakValue()
{
    return GetPeak().GetValue();
}

Pixel AdjoiningPixel::GetPeak()
{
    Pixel pixel;
    float max = 0.0;

    for (ulong i = 0; i < pixels_.size(); i++) {
        Pixel p = pixels_.at(i);
        if (i == 0  || max < p.GetValue()) {
            pixel = p;
            max = p.GetValue();
        }
    }

    return pixel;
}

void AdjoiningPixel::AddPixel(Pixel pixel)
{
    pixels_.push_back(pixel);
}

std::vector<AdjoiningPixel> AdjoiningPixel::Deblend(float base_step)
{
    std::vector<AdjoiningPixel> apList;

    if (pixels_.size() >= 10000) {
        apList.push_back(*this);
        return apList;
    }

    double radius = 1.0;

    while (pixels_.size() > 0) {
        Pixel peakPixel = GetPeak();

        // Extracts only pixels surrounding around the peak and
        // creates a new set of adjoining pixels.
        current_threshold_ = peakPixel.GetValue() - base_step * sqrt(peakPixel.GetValue() / base_step + 1.0);

        current_ap_ = new AdjoiningPixel();
        Extract(peakPixel.GetX(), peakPixel.GetY());
        cv::Point currentCenter = current_ap_->GetGravityCenter();

        // Determines if the current set of adjoining pixels is
        // a new peak or foot of already detected peak.
        AdjoiningPixel blendingAp;
        int blendingCount = 0;
        for (ulong i = 0 ; i < apList.size() ; i++) {
            AdjoiningPixel ap = apList.at(i);
            if (current_ap_->IsAdjoining(ap)) {
                blendingAp = ap;
                blendingCount++;
            } else {
                // When the distance between the two peaks is smaller than
                // the mean radius, they must not be separated.
                cv::Point center = ap.GetGravityCenter();
                float dist = sqrt((currentCenter.x - center.x) * (currentCenter.x - center.x) + (currentCenter.y - center.y) * (currentCenter.y - center.y));
                if (dist < radius) {
                    blendingAp = ap;
                    blendingCount++;
                }
            }
        }

        if (blendingCount == 0) {
            // New peak.
            apList.push_back(*current_ap_);
        } else if (blendingCount == 1) {
            // Foot of an already detected peak.
            for (ulong i = 0 ; i < current_ap_->GetPixels().size() ; i++) {
                Pixel pixel = current_ap_->GetPixels().at(i);
                blendingAp.AddPixel(pixel);
            }
        } else {
            // Foot of already detected several peaks.
            // In this case, now the pixels are ignored.
        }
    }

    //delete currentAp;

    return apList;
}

cv::Point AdjoiningPixel::GetGravityCenter()
{
    float x = 0.0;
    float y = 0.0;
    float w = 0.0;

    for (ulong i = 0 ; i < pixels_.size() ; i++) {
        Pixel p = pixels_.at(i);
        x += p.GetValue() * (float)p.GetX();
        y += p.GetValue() * (float)p.GetY();
        w += p.GetValue();
    }

    return cv::Point(x / w, y / w);
}

Star AdjoiningPixel::CreateStar()
{
    Star star;
    star.SetArea(pixels_.size());

    star.SetPeak(GetPeakValue());
    float starValue = 0.0;
    for (ulong i = 0; i < pixels_.size(); i++) {
        starValue += pixels_.at(i).GetValue();
    }
    star.SetValue(starValue);

    std::vector<Pixel> sortedPixels(pixels_);
    std::sort(sortedPixels.begin(), sortedPixels.end());

    float xAmount = 0.0;
    float yAmount = 0.0;
    float weight = 0.0;

    for (ulong i = 0; i < pixels_.size(); i++) {
        Pixel pixel = sortedPixels.at(i);
        xAmount += (pixel.GetX() + 0.5) * pixel.GetValue();
        yAmount += (pixel.GetY() + 0.5) * pixel.GetValue();
        weight += pixel.GetValue();
    }

    star.SetX(xAmount / weight);
    star.SetY(yAmount / weight);

    return star;
}

bool AdjoiningPixel::operator>(const AdjoiningPixel &other) const
{
    return pixels_.size() > other.GetPixels().size();
}

bool AdjoiningPixel::operator<(const AdjoiningPixel &other) const
{
    return pixels_.size() < other.GetPixels().size();
}

std::vector<Pixel> AdjoiningPixel::GetPixels() const
{
    return pixels_;
}

void AdjoiningPixel::SetPixels(const std::vector<Pixel> &value)
{
    pixels_ = value;
}

void AdjoiningPixel::Extract(int x, int y)
{
    for (ulong i = 0 ; i < pixels_.size() ; i++) {
        Pixel pixel = pixels_.at(i);

        if (pixel.GetX() == x  &&  pixel.GetY() == y  &&  pixel.GetValue() > current_threshold_) {
            current_ap_->AddPixel(pixel);
            pixels_.erase(pixels_.begin() + i);

            Extract(x, y-1);
            Extract(x-1, y);
            Extract(x+1, y);
            Extract(x, y+1);

            return;
        }
    }
}

bool AdjoiningPixel::IsAdjoining(AdjoiningPixel ap)
{
    for (ulong i = 0 ; i < pixels_.size() ; i++) {
        Pixel p1 = pixels_.at(i);
        for (ulong j = 0 ; j < ap.GetPixels().size() ; j++) {
            Pixel p2 = ap.GetPixels().at(j);
            if (abs(p1.GetX() - p2.GetX()) + abs(p1.GetY() - p2.GetY()) <= 1)
                return true;
        }
    }
    return false;
}
