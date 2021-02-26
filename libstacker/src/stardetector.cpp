#include "libstacker/stardetector.h"

#include "adjoiningpixel.h"
#include "hfti.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QDebug>
#include <QSettings>
#include <QTime>

#include <functional>
#include <string>
#include <string.h>
#include <stack>

#define THRESHOLD_COEFF 20.0

#define TOL		0.002   /* Default matching tolerance */
#define	NOBJS		40	/* Default number of objects */
#define MIN_MATCH	6	/* Min # of matched objects for transform */
#define MAX_MATCH	120	/* Max # of matches to use (~MAX_OBJS) */
#define MIN_OBJS	10	/* Min # of objects to use */
#define MAX_OBJS	100	/* Max # of objects to use */
#define	CLIP		3	/* Sigma clipping factor */
#define PM		57.2958 /* Radian to degree conversion */

namespace openskystacker {

class StarDetector::StarDetectorImpl
{
public:
    StarDetectorImpl();
    ~StarDetectorImpl();

    std::vector<Star> getStars(cv::Mat image, int thresholdCoeff);
    std::vector<Star> getStars(cv::Mat image);

    float getExtendedPixelValue(cv::Mat image, int x, int y);
    cv::Mat generateSkyBackground(cv::Mat image);

    using size_type = std::vector<Star>::size_type;
    void drawDetectedStars(const std::string& path, uint width, uint height, size_type limit, std::vector<Star> stars);

private:
    std::vector<AdjoiningPixel> getAdjoiningPixels(cv::Mat image, float threshold, float minPeak);
    AdjoiningPixel detectAdjoiningPixel(cv::Mat image, int x, int y, float threshold);
};

}

using namespace openskystacker;

StarDetector::StarDetector() : dPtr(new StarDetectorImpl)
{

}

StarDetector::~StarDetector()
{

}

std::vector<Star> StarDetector::getStars(cv::Mat image, int thresholdCoeff)
{
    return dPtr->getStars(image, thresholdCoeff);
}

std::vector<Star> StarDetector::getStars(cv::Mat image)
{
    return dPtr->getStars(image);
}

cv::Mat StarDetector::generateSkyBackground(cv::Mat image) {
    return dPtr->generateSkyBackground(image);
}

void StarDetector::drawDetectedStars(const std::string& path, uint width, uint height, size_type limit, std::vector<Star> stars)
{
    dPtr->drawDetectedStars(path, width, height, limit, stars);
}

float StarDetector::getExtendedPixelValue(cv::Mat image, int x, int y) {
    return dPtr->getExtendedPixelValue(image, x, y);
}










StarDetector::StarDetectorImpl::StarDetectorImpl()
{

}

StarDetector::StarDetectorImpl::~StarDetectorImpl()
{

}

std::vector<Star> StarDetector::StarDetectorImpl::getStars(cv::Mat image, int thresholdCoeff)
{
    cv::Mat imageGray;
    if (image.channels() == 1) {
        imageGray = image.clone();
    } else {
        imageGray = cv::Mat(image.rows, image.cols, CV_32FC1);
        cvtColor(image, imageGray, CV_BGR2GRAY);
    }

    cv::Mat skyImage = generateSkyBackground(imageGray);

    cv::Mat stars = imageGray - skyImage;
    cv::Rect bounds(stars.cols / 20, stars.rows / 20, stars.cols * 0.9, stars.rows * 0.9);
    // crop 1/10th off the edge in case edge pixels negatively affect the deviation
    cv::Mat thresholdImage = stars(bounds);

    cv::Scalar mean, stdDev;
    cv::meanStdDev(thresholdImage, mean, stdDev);

    float threshold = stdDev[0] * thresholdCoeff * 1.5;
    float minPeak = stdDev[0] * thresholdCoeff * 2.0;

    std::vector<AdjoiningPixel> apList = getAdjoiningPixels(stars, threshold, minPeak);

    std::vector<Star> allStars;
    for (ulong i = 0; i < apList.size(); i++) {
        AdjoiningPixel ap = apList.at(i);

        std::vector<AdjoiningPixel> deblendedApList = ap.deblend(threshold);

        for (ulong j = 0; j < deblendedApList.size(); j++) {
            AdjoiningPixel dap = deblendedApList.at(j);
            Star star = dap.createStar();
            allStars.push_back(star);
        }
    }

    return allStars;
}

std::vector<Star> StarDetector::StarDetectorImpl::getStars(cv::Mat image)
{
    QSettings settings("OpenSkyStacker", "OpenSkyStacker");
    float thresholdCoeff = settings.value("StarDetector/thresholdCoeff", THRESHOLD_COEFF).toFloat();

    return getStars(image, thresholdCoeff);
}

// TODO: ASSUMING GRAYSCALE 32 BIT FOR NOW
cv::Mat StarDetector::StarDetectorImpl::generateSkyBackground(cv::Mat image) {
    cv::Mat result = image.clone();

    cv::resize(result, result, cv::Size(result.cols/8, result.rows/8));

    int medianFilterSize = 5;
    cv::medianBlur(result, result, medianFilterSize);
    cv::resize(result, result, cv::Size(image.cols, image.rows));

    int gaussianFilterSize = 31;
    cv::GaussianBlur(result, result, cv::Size(gaussianFilterSize, gaussianFilterSize), 0);

    return result;
}

void StarDetector::StarDetectorImpl::drawDetectedStars(const std::string& path, uint width, uint height, size_type limit, std::vector<Star> stars)
{
    cv::Mat output = cv::Mat::zeros(height, width, CV_8UC3);
    const int maxRadius = 30;

    std::sort(stars.begin(), stars.end(), std::greater<Star>());
    float maxValue = stars.at(0).value;

    for (size_type i = 0; i < stars.size() && i < limit; i++) {
        Star star = stars.at(i);
        float ratio = star.value / maxValue;
        int radius = maxRadius * ratio;

        cv::circle(output, cv::Point(star.x,star.y),radius,cv::Scalar(255,255,255),-1);
    }

    cv::imwrite(path, output);
}

std::vector<AdjoiningPixel> StarDetector::StarDetectorImpl::getAdjoiningPixels(cv::Mat image, float threshold, float minPeak)
{
    std::vector<AdjoiningPixel> list;

    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            if (image.at<float>(y,x) > threshold) {
                AdjoiningPixel ap = detectAdjoiningPixel(image, x, y, threshold);

                if (ap.getPeakValue() > minPeak){
                    list.push_back(ap);
                }

            }
        }
    }

    return list;
}

AdjoiningPixel StarDetector::StarDetectorImpl::detectAdjoiningPixel(cv::Mat image, int x, int y, float threshold)
{
    AdjoiningPixel ap;
    std::stack<Pixel> stack;
    stack.push(Pixel(x, y, image.at<float>(y, x)));

    while (stack.empty() == false) {
        Pixel pixel = stack.top(); stack.pop();

        x = pixel.x;
        y = pixel.y;

        if (image.at<float>(y, x) > threshold) {
            ap.addPixel(pixel);

            // The pixel value is cleared.
            image.at<float>(y, x) = threshold - 1;

            if (0 <= x  &&  x < image.cols  &&
                0 <= y-1  &&  y-1 < image.rows)
                stack.push(Pixel(x, y-1, image.at<float>(y-1, x)));
            if (0 <= x-1  &&  x-1 < image.cols  &&
                0 <= y  &&  y < image.rows)
                stack.push(Pixel(x-1, y, image.at<float>(y, x-1)));
            if (0 <= x+1  &&  x+1 < image.cols  &&
                0 <= y  &&  y < image.rows)
                stack.push(Pixel(x+1, y, image.at<float>(y, x+1)));
            if (0 <= x  &&  x < image.cols  &&
                0 <= y+1  &&  y+1 < image.rows)
                stack.push(Pixel(x, y+1, image.at<float>(y+1, x)));
        }
    }

    return ap;
}

float StarDetector::StarDetectorImpl::getExtendedPixelValue(cv::Mat image, int x, int y) {
    if (x < 0) x = 0;
    if (x >= image.cols) x = image.cols - 1;
    if (y < 0) y = 0;
    if (y >= image.rows) y = image.rows - 1;

    return image.at<float>(y, x);
}
