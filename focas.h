#ifndef FOCAS_H
#define FOCAS_H

#include "opencv2/core.hpp"

void h12(int mode, int lpivot, int l1, int m, int u, cv::Mat u, int iue, std::vector<float> up,
         cv::Mat c, int ice, int icv, int ncv);

void hfti(cv::Mat a, int mda, int m, int n, cv::Mat b, int mdb, int nb, float tau,
          int krank, float rnorm[], float h[], float g[], int ip[]);

cv::Mat mktransform(cv::Mat target, cv::Mat reference, float tolerance, int numObj);

#endif // FOCAS_H
