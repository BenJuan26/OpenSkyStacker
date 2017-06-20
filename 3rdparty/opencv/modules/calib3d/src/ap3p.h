#ifndef P3P_P3P_H
#define P3P_P3P_H

#include "precomp.hpp"

namespace cv {
class ap3p {
private:
    template<typename T>
    void init_camera_parameters(const cv::Mat &cameraMatrix) {
        cx = cameraMatrix.at<T>(0, 2);
        cy = cameraMatrix.at<T>(1, 2);
        fx = cameraMatrix.at<T>(0, 0);
        fy = cameraMatrix.at<T>(1, 1);
    }

    template<typename OpointType, typename IpointType>
    void extract_points(const cv::Mat &opoints, const cv::Mat &ipoints, std::vector<double> &points) {
        points.clear();
        points.resize(20);
        for (int i = 0; i < 4; i++) {
            points[i * 5] = ipoints.at<IpointType>(i).x * fx + cx;
            points[i * 5 + 1] = ipoints.at<IpointType>(i).y * fy + cy;
            points[i * 5 + 2] = opoints.at<OpointType>(i).x;
            points[i * 5 + 3] = opoints.at<OpointType>(i).y;
            points[i * 5 + 4] = opoints.at<OpointType>(i).z;
        }
    }

    void init_inverse_parameters();

    double fx, fy, cx, cy;
    double inv_fx, inv_fy, cx_fx, cy_fy;
public:
    ap3p() {}

    ap3p(double fx, double fy, double cx, double cy);

    ap3p(cv::Mat cameraMatrix);

    bool solve(cv::Mat &R, cv::Mat &tvec, const cv::Mat &opoints, const cv::Mat &ipoints);

    int solve(double R[4][3][3], double t[4][3],
              double mu0, double mv0, double X0, double Y0, double Z0,
              double mu1, double mv1, double X1, double Y1, double Z1,
              double mu2, double mv2, double X2, double Y2, double Z2);

    bool solve(double R[3][3], double t[3],
               double mu0, double mv0, double X0, double Y0, double Z0,
               double mu1, double mv1, double X1, double Y1, double Z1,
               double mu2, double mv2, double X2, double Y2, double Z2,
               double mu3, double mv3, double X3, double Y3, double Z3);

    // This algorithm is from "Tong Ke, Stergios Roumeliotis, An Efficient Algebraic Solution to the Perspective-Three-Point Problem" (Accepted by CVPR 2017)
    // See https://arxiv.org/pdf/1701.08237.pdf
    // featureVectors: 3 bearing measurements (normalized) stored as column vectors
    // worldPoints: Positions of the 3 feature points stored as column vectors
    // solutionsR: 4 possible solutions of rotation matrix of the world w.r.t the camera frame
    // solutionsT: 4 possible solutions of translation of the world origin w.r.t the camera frame
    int computePoses(const double featureVectors[3][3], const double worldPoints[3][3], double solutionsR[4][3][3],
                     double solutionsT[4][3]);

};
}
#endif //P3P_P3P_H
