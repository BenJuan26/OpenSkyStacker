#include "stardetector.h"

#define THRESHOLD_COEFF 2.0


StarDetector::StarDetector()
{

}

StarDetector::~StarDetector()
{

}

void StarDetector::process(cv::Mat image)
{
    cv::Mat imageGray(image.rows, image.cols, CV_32FC1);
    cvtColor(image, imageGray, CV_BGR2GRAY);

    cv::Mat skyImage = generateSkyBackground(imageGray);

    cv::Mat stars = imageGray - skyImage;
    cv::Rect bounds(stars.cols / 20, stars.rows / 20, stars.cols * 0.9, stars.rows * 0.9);
    // crop 1/10th off the edge in case edge pixels negatively affect the deviation
    cv::Mat thresholdImage = stars(bounds);

    cv::Scalar mean, stdDev;
    cv::meanStdDev(thresholdImage, mean, stdDev);
    float threshold = stdDev[0] * THRESHOLD_COEFF * 1.5;
    float minPeak = stdDev[0] * THRESHOLD_COEFF * 2.0;

    std::vector<AdjoiningPixel> apList = getAdjoiningPixels(stars, threshold, minPeak);

    std::vector<Star> allStars;
    for (ulong i = 0; i < apList.size(); i++) {
        AdjoiningPixel ap = apList.at(i);

        Star star = ap.createStar();
        allStars.push_back(star);


//        Pixel pixel = ap.getPeak();
//        qDebug() << "Star peak at" << pixel.getX() << "," << pixel.getY() << ", area" << ap.getPixels().size();
/*
        std::vector<AdjoiningPixel> deblendedApList = ap.deblend(threshold);

        for (ulong j = 0; j < deblendedApList.size(); j++) {
            AdjoiningPixel dap = deblendedApList.at(j);
            Star star = dap.createStar();
            allStars.push_back(star);
        }
*/
    }

    qDebug() << "Top 20 stars:";
    std::sort(allStars.begin(), allStars.end());
    for (ulong i = allStars.size() - 1; i >= 0 && i > allStars.size() - 21; i--) {
        Star star = allStars.at(i);
        qDebug() << star.getX() << "," << star.getY() << ":" << star.getValue();
    }
}

// TODO: ASSUMING GRAYSCALE 32 BIT FOR NOW
cv::Mat StarDetector::generateSkyBackground(cv::Mat image) {
    cv::Mat result = image.clone();

    cv::resize(result, result, cv::Size(result.cols/8, result.rows/8));

//    cv::Mat resizeColor(result.rows, result.cols, CV_32FC3);
//    cv::cvtColor(result, resizeColor, CV_GRAY2BGR);
//    imwrite("/Users/Ben/Pictures/OpenSkyStacker/resized.tif", resizeColor);

    cv::medianBlur(result, result, 5);
    cv::resize(result, result, cv::Size(image.cols, image.rows));

    float *buffer;
    buffer = new float[result.cols];
    int filter_size = 16;

    for (int y = 0; y < result.rows; y++) {
        float val = 0.0;
        for (int x = - (filter_size - 1) / 2 - 1 ; x < filter_size / 2 ; x++)
            val += getExtendedPixelValue(result, x, y);

        for (int x = 0 ; x < result.cols ; x++) {
            val -= getExtendedPixelValue(result, x - (filter_size - 1) / 2 - 1, y);
            val += getExtendedPixelValue(result, x + filter_size / 2, y);

            buffer[x] = val / (float)filter_size;
        }

        for (int x = 0 ; x < result.cols ; x++)
            result.at<float>(y, x) = buffer[x];
    }

    delete [] buffer;

    buffer = new float[result.rows];

    for (int x = 0 ; x < result.cols ; x++) {
        float val = 0.0;
        for (int y = - (filter_size - 1) / 2 - 1 ; y < filter_size / 2 ; y++)
            val += getExtendedPixelValue(result, x, y);

        for (int y = 0 ; y < result.rows ; y++) {
            val -= getExtendedPixelValue(result, x, y - (filter_size - 1) / 2 - 1);
            val += getExtendedPixelValue(result, x, y + filter_size / 2);

            buffer[y] = val / (float)filter_size;
        }

        for (int y = 0 ; y < result.rows ; y++)
            result.at<float>(y, x) = buffer[y];
    }

    delete [] buffer;

    return result;
}

std::vector<AdjoiningPixel> StarDetector::getAdjoiningPixels(cv::Mat image, float threshold, float minPeak)
{
    std::vector<AdjoiningPixel> list;

    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            //qDebug() << "Getting adjoining pixels at" << x << "," << y;
            if (image.at<float>(y,x) > threshold) {
                AdjoiningPixel ap = detectAdjoiningPixel(image, x, y, threshold);
                //qDebug() << "Got adjoining pixel at" << x << "," << y;
                if (ap.getPeakValue() > minPeak){
                    list.push_back(ap);
                    qDebug() << "List size:" << list.size();
                }

            }
        }
    }

    qDebug() << "Final list size:" << list.size();

    return list;
}

AdjoiningPixel StarDetector::detectAdjoiningPixel(cv::Mat image, int x, int y, float threshold)
{
    AdjoiningPixel ap;
    std::stack<Pixel> stack;
    stack.push(Pixel(x, y, image.at<float>(y, x)));

    while (stack.empty() == false) {
        Pixel pixel = stack.top(); stack.pop();

        x = pixel.getX();
        y = pixel.getY();

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

float StarDetector::getExtendedPixelValue(cv::Mat image, int x, int y) {
    //qDebug() << "getting pixel" << x << y;
    if (x < 0) x = 0;
    if (x >= image.cols) x = image.cols - 1;
    if (y < 0) y = 0;
    if (y >= image.rows) y = image.rows - 1;

    return image.at<float>(y, x);
}
