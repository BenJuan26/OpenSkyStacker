#include "stardetector.h"
#include "hfti.h"
#include <QTime>
#include <functional>

#define THRESHOLD_COEFF 20.0

#define TOL		0.002   /* Default matching tolerance */
#define	NOBJS		40	/* Default number of objects */
#define MIN_MATCH	6	/* Min # of matched objects for transform */
#define MAX_MATCH	120	/* Max # of matches to use (~MAX_OBJS) */
#define MIN_OBJS	10	/* Min # of objects to use */
#define MAX_OBJS	100	/* Max # of objects to use */
#define	CLIP		3	/* Sigma clipping factor */
#define PM		57.2958 /* Radian to degree conversion */


StarDetector::StarDetector()
{

}

StarDetector::~StarDetector()
{

}

std::vector<Star> StarDetector::GetStars(cv::Mat image)
{
    cv::Mat imageGray(image.rows, image.cols, CV_32FC1);
    cvtColor(image, imageGray, CV_BGR2GRAY);

    cv::Mat skyImage = GenerateSkyBackground(imageGray);

    cv::Mat stars = imageGray - skyImage;
    cv::Rect bounds(stars.cols / 20, stars.rows / 20, stars.cols * 0.9, stars.rows * 0.9);
    // crop 1/10th off the edge in case edge pixels negatively affect the deviation
    cv::Mat thresholdImage = stars(bounds);

    cv::Scalar mean, stdDev;
    cv::meanStdDev(thresholdImage, mean, stdDev);
    float threshold = stdDev[0] * THRESHOLD_COEFF * 1.5;
    float minPeak = stdDev[0] * THRESHOLD_COEFF * 2.0;

    std::vector<AdjoiningPixel> apList = GetAdjoiningPixels(stars, threshold, minPeak);
    qDebug() << "Total adjoining pixels:" << apList.size();

    std::vector<Star> allStars;
    for (ulong i = 0; i < apList.size(); i++) {
        AdjoiningPixel ap = apList.at(i);

        std::vector<AdjoiningPixel> deblendedApList = ap.Deblend(threshold);

        for (ulong j = 0; j < deblendedApList.size(); j++) {
            AdjoiningPixel dap = deblendedApList.at(j);
            Star star = dap.CreateStar();
            allStars.push_back(star);
        }
    }
    //QString name = "/home/ben/Desktop/stars" + QString::number(QTime::currentTime().msec()) + ".tif";
    //drawDetectedStars(name.toUtf8().constData(), image.cols, image.rows, -1, allStars);
    return allStars;
}

// TODO: ASSUMING GRAYSCALE 32 BIT FOR NOW
cv::Mat StarDetector::GenerateSkyBackground(cv::Mat image) {
    cv::Mat result = image.clone();

    cv::resize(result, result, cv::Size(result.cols/8, result.rows/8));
    cv::medianBlur(result, result, 5);
    cv::resize(result, result, cv::Size(image.cols, image.rows));

    float *buffer;
    buffer = new float[result.cols];
    int filter_size = 16;

    for (int y = 0; y < result.rows; y++) {
        float val = 0.0;
        for (int x = - (filter_size - 1) / 2 - 1 ; x < filter_size / 2 ; x++)
            val += GetExtendedPixelValue(result, x, y);

        for (int x = 0 ; x < result.cols ; x++) {
            val -= GetExtendedPixelValue(result, x - (filter_size - 1) / 2 - 1, y);
            val += GetExtendedPixelValue(result, x + filter_size / 2, y);

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
            val += GetExtendedPixelValue(result, x, y);

        for (int y = 0 ; y < result.rows ; y++) {
            val -= GetExtendedPixelValue(result, x, y - (filter_size - 1) / 2 - 1);
            val += GetExtendedPixelValue(result, x, y + filter_size / 2);

            buffer[y] = val / (float)filter_size;
        }

        for (int y = 0 ; y < result.rows ; y++)
            result.at<float>(y, x) = buffer[y];
    }

    delete [] buffer;

    return result;
}

void StarDetector::DrawDetectedStars(const std::string& path, uint width, uint height, int limit, std::vector<Star> stars)
{
    if (limit < 0) limit = stars.size();

    cv::Mat output = cv::Mat::zeros(height, width, CV_8UC3);
    const int maxRadius = 30;

    std::sort(stars.begin(), stars.end(), std::greater<Star>());
    float maxValue = stars.at(0).GetValue();
    for (int i = 0; i < stars.size() && i < limit; i++) {
        Star star = stars.at(i);
        float ratio = star.GetValue() / maxValue;
        int radius = maxRadius * ratio;

        cv::circle(output, cv::Point(star.GetX(),star.GetY()),radius,cv::Scalar(255,255,255),-1);
    }

    cv::imwrite(path, output);
}

void StarDetector::test()
{
    short matches[3][MAX_MATCH];
    int	m = 12;
    std::vector<Star> List1;
    std::vector<Star> List2;
    float	xfrm[2][3];

    List1.push_back(Star(38, 30, 39));
    List1.push_back(Star(603, 1541, 22));
    List1.push_back(Star(3123, 1788, 22));
    List1.push_back(Star(3564, 1202, 18));
    List1.push_back(Star(577, 1084, 16));
    List1.push_back(Star(2042, 1597, 12));
    List1.push_back(Star(259, 608, 12));
    List1.push_back(Star(2873, 1597, 12));
    List1.push_back(Star(2956, 2809, 11));
    List1.push_back(Star(3781, 291, 11));
    List1.push_back(Star(2610, 1947, 11));
    List1.push_back(Star(3953, 331, 10));

    List2.push_back(Star(276, 93, 38));
    List2.push_back(Star(845, 1598, 22));
    List2.push_back(Star(3364, 1835, 26));
    List2.push_back(Star(3805, 1245, 20));
    List2.push_back(Star(817, 1142, 16));
    List2.push_back(Star(2282, 1648, 14));
    List2.push_back(Star(498, 669, 12));
    List2.push_back(Star(3113, 1617, 13));
    List2.push_back(Star(3203, 2857, 12));
    List2.push_back(Star(4018, 333, 12));
    List2.push_back(Star(2852, 1995, 12));
    List2.push_back(Star(4191, 372, 11));

    int	i, j, i1, i2;
    int	mda = MAX_MATCH, mdb = MAX_MATCH, n = 3, nb = 2;
    int	krank, ip[3];
    float	tau = 0.1, rnorm[2], h[3], g[3];
    float	a[3][MAX_MATCH], b[2][MAX_MATCH];
    float	x, y, r2, sum, rms;

    /* Require a minimum number of points. */
    if (m < MIN_MATCH) {
        printf ("Match not found: use more objects or larger tolerance\n");
        exit (0);
    }

    /* Compute the initial transformation with the 12 best matches. */
    j = (m < 12) ? m : 12;
    for (i=0; i<j; i++) {
        matches[0][i] = i;
        matches[1][i] = i;
        a[0][i] = List1[matches[0][i]].GetX();
        a[1][i] = List1[matches[0][i]].GetY();
        a[2][i] = 1.;
        b[0][i] = List2[matches[1][i]].GetX();
        b[1][i] = List2[matches[1][i]].GetY();
    }

    hfti_(a, &mda, &j, &n, b, &mdb, &nb, &tau, &krank, rnorm, h, g, ip);
    for (i=0; i<2; i++)
        for (j=0; j<3; j++)
        xfrm[i][j] = b[i][j];

    /* Start with all matches compute RMS and reject outliers.	      */
    /* The outliers are found from the 60% point in the sorted residuals. */
    for (;;) {
        sum = 0.;
        for (i=0; i<m; i++) {
        i1 = matches[0][i];
        i2 = matches[1][i];
        r2 = List1[i1].GetX();
        y = List1[i1].GetY();
        x = xfrm[0][0] * r2 + xfrm[0][1] * y + xfrm[0][2];
        y = xfrm[1][0] * r2 + xfrm[1][1] * y + xfrm[1][2];
        x -= List2[i2].GetX();
        y -= List2[i2].GetY();
        r2 = x * x + y * y;
        for (j=i; j>0 && r2<a[1][j-1]; j--)
            a[1][j] = a[1][j-1];
        a[0][i] = r2;
        a[1][j] = r2;
        sum += r2;
        }

        /* Set clipping limit and quit when no points are clipped. */
        i = 0.6 * m;
        r2 = CLIP * a[1][i];
        if (r2 >= a[1][m-1]) {
        rms = sqrt(sum / m);
        break;
        }

        /* Clip outliers and redo the fit. */
        j = 0;
        for (i=0; i<m; i++) {
        if (a[0][i] < r2) {
            i1 = matches[0][i];
            i2 = matches[1][i];
            matches[0][j] = i1;
            matches[1][j] = i2;
            a[0][j] = List1[i1].GetX();
            a[1][j] = List1[i1].GetY();
            a[2][j] = 1.;
            b[0][j] = List2[i2].GetX();
            b[1][j] = List2[i2].GetY();
            j++;
        }
        }
        m = j;

//	    if (m < MIN_MATCH) {
//		printf (
//		    "Match not found: use more objects or larger tolerance\n");
//		exit (0);
//	    }

        hfti_(a, &mda, &m, &n, b, &mdb, &nb, &tau, &krank, rnorm, h, g,ip);

        for (i=0; i<2; i++)
        for (j=0; j<3; j++)
            xfrm[i][j] = b[i][j];
    }

    qDebug() << xfrm[0][0] << xfrm[0][1] << xfrm[0][2];
    qDebug() << xfrm[1][0] << xfrm[1][1] << xfrm[1][2];

    printf ("Number of matches = %d, RMS of fit = %8.2f\n", m, rms);
}

std::vector<AdjoiningPixel> StarDetector::GetAdjoiningPixels(cv::Mat image, float threshold, float minPeak)
{
    std::vector<AdjoiningPixel> list;

    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            if (image.at<float>(y,x) > threshold) {
                AdjoiningPixel ap = DetectAdjoiningPixel(image, x, y, threshold);

                if (ap.GetPeakValue() > minPeak){
                    list.push_back(ap);
                }

            }
        }
    }

    return list;
}

AdjoiningPixel StarDetector::DetectAdjoiningPixel(cv::Mat image, int x, int y, float threshold)
{
    AdjoiningPixel ap;
    std::stack<Pixel> stack;
    stack.push(Pixel(x, y, image.at<float>(y, x)));

    while (stack.empty() == false) {
        Pixel pixel = stack.top(); stack.pop();

        x = pixel.GetX();
        y = pixel.GetY();

        if (image.at<float>(y, x) > threshold) {
            ap.AddPixel(pixel);

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

float StarDetector::GetExtendedPixelValue(cv::Mat image, int x, int y) {
    //qDebug() << "getting pixel" << x << y;
    if (x < 0) x = 0;
    if (x >= image.cols) x = image.cols - 1;
    if (y < 0) y = 0;
    if (y >= image.rows) y = image.rows - 1;

    return image.at<float>(y, x);
}
