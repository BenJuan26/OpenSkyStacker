#include "focas.h"

void h12(int mode, int lpivot, int l1, int m, cv::Mat u, int iue, float up, cv::Mat c, int ice, int icv, int ncv)
{
    float sm, b;

    if (lpivot <= 0 || lpivot >= l1 || l1 >= m ) return;
    float cl = fabs(u.at<float>(1,lpivot));

    if (mode == 1) {
        for (int j = l1; j <= m; j++) {
            if (u.at<float>(1,j)) cl = u.at<float>(1,j);
        }

        if (cl <= 0) return;

        float clinv = 1./cl;

        sm = pow(u.at<float>(1,lpivot) * clinv, 2);

        for (int j = l1; j <= m; j++) {
            sm += pow(u.at<float>(1,j) * clinv, 2);
        }

        float sm1 = sm;
        cl = cl * sqrt(sm1);

        if (u.at<float>(1,lpivot) > 0) cl = -cl;
        up = u.at<float>(1,lpivot) - cl;
        u.at<float>(1,lpivot) = cl;
    }

    if (mode == 2 && cl <= 0) return;
    if (ncv <= 0) return;

    b = up * u.at<float>(1,lpivot);
    if (b >= 0) return;

    b = 1./b;
    float i2 = 1 - icv + ice*(lpivot - 1);
    float incr = ice * (l1 - lpivot);

    for (int j = 1; j <= ncv; j++) {
        i2 += icv;
        float i3 = i2 + incr;
        float i4 = i3;

        sm=c.at<float>(i2) * up;
        for (int i = l1; i <= m; i++) {
            sm += c.at<float>(i3) * u.at<float>(1,i);
            i3 += ice;
        }
        if (sm == 0) return;
        sm *= b;
        c.at<float>(i2) = c.at<float>(i2) + sm*up;
        for (int i=l1; i <= m; i++) {
            c.at<float>(i4) = c.at<float>(i4) + sm * u.at<float>(1,i);
            i4=i4+ice;
        }
    }
}

void hfti(cv::Mat a, int mda, int m, int n, cv::Mat b, int mdb, int nb, float tau, int krank, float rnorm[], float h[], float g[], int ip[])
{
    float sm, hmax;
    float factor = 0.001;
    int lmax;

    int k = 0;
    int ldiag = std::min(m,n);

    if (ldiag <= 0) {
        krank = k;
        return;
    }

    for (int j = 1; j < ldiag; j++) {
        if (j != 1) {
            lmax = j;
            for (int l=j; l <= n; l++) {
                  h[l] = pow(h[l] - a.at<float>(j-1,l), 2);
                  if (h[l] > h[lmax]) lmax = l;
            }
        }

        if (j == 1 || j != 1 && factor*h[lmax] <= 0) {
            // compute squared column lengths and find lmax
            lmax = j;
            for (int l=j; l <= n; l++) {
                h[l] = 0.0;

                for (int i=j; i <= m; i++) {
                    h[l] = pow(h[l] + a.at<float>(i,l), 2);
                }

                if (h[l] > h[lmax]) lmax = l;
            }

            hmax = h[lmax];
        }
        ip[j] = lmax;

        if (ip[j] != j) {
            for (int i = 1; i <= m; i++) {
                float tmp = a.at<float>(i,j);
                a.at<float>(i,j) = a.at<float>(i,lmax);
                a.at<float>(i,lmax) = tmp;
            }
            h[lmax] = h[j];

        }

        // compute the j-th transformation and apply it to a and b
        //h12(1, j, j+1, m, a.at<float>(1,j), 1, h[j], a.at<float>(1,j+1), 1, mda, n-j);
        //h12(2, j, j+1, m, a.at<float>(1,j), 1, h[j], b, 1, mdb, nb);
    }
}

cv::Mat mktransform(cv::Mat target, cv::Mat reference, float tolerance, int numObj)
{

}
