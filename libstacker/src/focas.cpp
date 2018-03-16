#include "focas.h"

using namespace openskystacker;

std::vector<Triangle> openskystacker::GenerateTriangleList(std::vector<Star> List)
{
    int nobjs = NOBJS;
    if (List.size() < NOBJS) nobjs = static_cast<int>(List.size());

    //ntriang = (nobjs - 2) * (nobjs - 1) * nobjs / 6;
    std::vector<Triangle> List_triang; //(ntriang);

    int	i, j, k, p=0;
    float	di, dj, dk, a, b, c, h1, h2;

    std::vector<float> sides(nobjs * (nobjs - 1)/2 + 1);

    /* Minimize the number of triangle sides to compute */
    for (i = 0; i < nobjs; i++) {
        for (j = i + 1; j < nobjs; j++) {
            h1 = (List[i].x - List[j].x);
            h2 = (List[i].y - List[j].y);
            k = SidesPos(i, j, nobjs);
            sides[k] = sqrt(h1*h1 + h2*h2);
        }
    }

    /* Order the triangle sides and compute the triangle space coords */
    for (i = 0; i < (nobjs-2); i++) {
        for (j = i+1; j < (nobjs-1); j++) {
            for (k = j+1; k < nobjs; k++) {
                di = sides [SidesPos(i, j, nobjs)] ;
                dj = sides [SidesPos(j, k, nobjs)] ;
                dk = sides [SidesPos(k, i, nobjs)] ;

                if (dk > dj) {
                    if (dk > di) {
                        if (dj > di) {	/* kji */
                            a = dk; b = dj; c = di;
                            List_triang.push_back(Triangle(k,i,j));
                        } else {		/* kij */
                            a = dk; b = di; c = dj;
                            List_triang.push_back(Triangle(i,k,j));
                        }
                    } else {		/* ikj */
                        a = di; b = dk; c = dj;
                        List_triang.push_back(Triangle(i,j,k));
                    }
                } else if (dj > di) {
                    if (di > dk) {		/* jik */
                        a = dj; b = di; c = dk;
                        List_triang.push_back(Triangle(j,k,i));
                    } else {		/* jki */
                        a = dj; b = dk; c = di;
                        List_triang.push_back(Triangle(k,j,i));
                    }
                } else {			/* ijk */
                    a = di; b = dj; c = dk;
                    List_triang.push_back(Triangle(j,i,k));
                }

                /* Include only triangles with b/a < 0.9 */
                if ( b/a < 0.9 ) {
                    List_triang[p].x = b/a;
                    List_triang[p].y = c/a;
                    p++;
                }
                else {
                    List_triang.pop_back();
                }
            }
        }
    }

    return List_triang;
}

int openskystacker::SidesPos(int i, int j, int n)
{
    if (i < j)
        return(i*(2*n-i-3)/2 + j);
    else
        return(j*(2*n-j-3)/2 + i);
}

std::vector<std::vector<int> > openskystacker::FindMatches(int nobjs, int *k_, std::vector<Triangle> List_triangA, std::vector<Triangle> List_triangB)
{
    std::vector<std::vector<int> > matches(3, std::vector<int>(MAX_MATCH, 0));
    int	i, j, l, n, first, last;
    int &k = *k_;
    k = 0;

    int *Table_match = new int[nobjs * nobjs];

    for (int i = 0; i < nobjs * nobjs; i++) {
        Table_match[i] = 0;
    }

    /* Sort List_triangB by x coordinate. */
    SortTriangles(&List_triangB, 0, static_cast<int>(List_triangB.size()) - 1);

    /* Find objects within tolerance distance in triangle space. */
    for (Triangle tri : List_triangA) {
        BinSearchTriangles(tri.x, &List_triangB, &first, &last);
        CheckTolerance(nobjs, tri, &List_triangB, first, last, Table_match);
    }

    /* Find the nobjs points with the most matches. */
    k = 0;
    for (i = 0; i < nobjs; i++) {
        for (j = 0; j < nobjs; j++) {
            n = Table_match[i*nobjs+j];
            if (n > 0) {
                if (k < nobjs) {
                    for (l=k; l>0 && n>matches[2][l-1]; l--) {
                        matches[0][l] = matches[0][l-1];
                        matches[1][l] = matches[1][l-1];
                        matches[2][l] = matches[2][l-1];
                    }
                    matches[0][l] = i;
                    matches[1][l] = j;
                    matches[2][l] = n;
                    k++;
                } else if (n >= matches[2][nobjs-1]) {
                    for (l=k; l>0 && n>matches[2][l-1]; l--) {
                        matches[0][l] = matches[0][l-1];
                        matches[1][l] = matches[1][l-1];
                        matches[2][l] = matches[2][l-1];
                    }
                    matches[0][l] = i;
                    matches[1][l] = j;
                    matches[2][l] = n;
                    l = k < MAX_MATCH ? k + 1: k;
                    n = matches[2][nobjs-1];
                    for (k=nobjs; k<l && n==matches[2][k]; k++);
                }
            }
        }
    }

    delete [] Table_match;

    return matches;
}

void openskystacker::SortTriangles(std::vector<Triangle> *List_Triang_, int l, int r)
{
    // make index operator easier
    std::vector<Triangle> &List_Triang = *List_Triang_;

    Triangle v;
    Triangle t;
    int	i, j;

    if( r > l ) {
        v.x = List_Triang[r].x;
        i = l-1;
        j = r;

        for(;;) {
            while(List_Triang[++i].x < v.x );
            while((j > 1) && (List_Triang[--j].x > v.x));
            if(i >= j)
                break;

            t.x = List_Triang[i].x;
            List_Triang[i].x = List_Triang[j].x;
            List_Triang[j].x = t.x;

            t.y = List_Triang[i].y;
            List_Triang[i].y = List_Triang[j].y;
            List_Triang[j].y = t.y;

            t.s1 = List_Triang[i].s1;
            List_Triang[i].s1 = List_Triang[j].s1;
            List_Triang[j].s1 = t.s1;

            t.s2 = List_Triang[i].s2;
            List_Triang[i].s2 = List_Triang[j].s2;
            List_Triang[j].s2 = t.s2;

            t.s3 = List_Triang[i].s3;
            List_Triang[i].s3 = List_Triang[j].s3;
            List_Triang[j].s3 = t.s3;
        }

        t.x = List_Triang[i].x;
        List_Triang[i].x = List_Triang[r].x;
        List_Triang[r].x = t.x;

        t.y = List_Triang[i].y;
        List_Triang[i].y = List_Triang[r].y;
        List_Triang[r].y = t.y;

        t.s1= List_Triang[i].s1;
        List_Triang[i].s1 = List_Triang[r].s1;
        List_Triang[r].s1 = t.s1;

        t.s2 = List_Triang[i].s2;
        List_Triang[i].s2 = List_Triang[r].s2;
        List_Triang[r].s2 = t.s2;

        t.s3 = List_Triang[i].s3;
        List_Triang[i].s3 = List_Triang[r].s3;
        List_Triang[r].s3 = t.s3;

        SortTriangles(&List_Triang, l, i-1);
        SortTriangles(&List_Triang, i+1, r);
    }
}

void openskystacker::BinSearchTriangles(float key, std::vector<Triangle> *List_triang_, int *first, int *last)
{
    std::vector<Triangle> &List_triang = *List_triang_;
    int ntriang = static_cast<int>(List_triang.size());
    int	min = 0, max = ntriang - 1, middle ;
    int	found = 0, i;
    double tolerance = TOL;

    while ((!found) && (max - min > 1)) {
        middle  = (min + max ) / 2;
        if (fabs(List_triang[middle].x - key) < tolerance)
        found = 1;
        else if ( key < ( List_triang[middle ].x - tolerance))
        max = middle ;
        else if ( key > (List_triang[middle ].x + tolerance))
        min = middle;
    }

    /* Not found */
    if (!found) {
        *first = 2;
        *last  = 1;
        return;
    }

    for (i = middle; i > 0; i--) {
        if (fabs (List_triang[i].x - key) > tolerance)
        break;
    }
    *first = i;

    for (i = middle; i < ntriang-1; i++) {
        if (fabs (List_triang[i].x - key) > tolerance)
        break;
    }
    *last = i;

}

void openskystacker::CheckTolerance(int nobjs, Triangle List_triangA, std::vector<Triangle> *List_triangB_, int first, int last, int Table_match[])
{
    std::vector<Triangle> &List_triangB = *List_triangB_;
    double tolerance = TOL;
    double tolerance2 = TOL * TOL;
    float	temp1, temp2, distance;
    int	i, h1, h2;

    for (i = first ; i <= last; i++) {
        temp2 = (List_triangA.y - List_triangB[i].y);
        if (temp2 < tolerance) {
            temp1 = (List_triangA.x - List_triangB[i].x);
            distance = temp1 * temp1 + temp2 * temp2;
            if (distance < tolerance2) {
                h1 = List_triangA.s1;
                h2 = List_triangB[i].s1;
                Table_match[h1*nobjs+h2]++;

                h1 = List_triangA.s2;
                h2 = List_triangB[i].s2;
                Table_match[h1*nobjs+h2]++;

                h1 = List_triangA.s3;
                h2 = List_triangB[i].s3;
                Table_match[h1*nobjs+h2]++;
            }
        }
    }
}

std::vector<std::vector<float> > openskystacker::FindTransform(std::vector<std::vector<int> > matches,
        int m, std::vector<Star> List1, std::vector<Star> List2, int *ok)
{
    //float	xfrm[2][3];
    std::vector<std::vector<float> > xfrm(2, std::vector<float>(3,0));

    int	i, j, i1, i2;
    int	krank, ip[3];
    float	tau = 0.1f, rnorm[2], h[3], g[3];
    cv::Mat	a(3, MAX_MATCH, CV_32FC1);
    cv::Mat b(2, MAX_MATCH, CV_32FC1);
    float	x, y, r2, sum, rms;

    /* Require a minimum number of points. */
    if (m < MIN_MATCH) {
        if (ok)
            *ok = -1;
        return xfrm;
    }

    /* Compute the initial transformation with the 12 best matches. */
    j = (m < 12) ? m : 12;
    for (i=0; i<j; i++) {
        a.at<float>(0,i) = List1[matches[0][i]].x;
        a.at<float>(1,i) = List1[matches[0][i]].y;
        a.at<float>(2,i) = 1.;
        b.at<float>(0,i) = List2[matches[1][i]].x;
        b.at<float>(1,i) = List2[matches[1][i]].y;
    }

    hfti(a, j, b, tau, krank, rnorm, h, g, ip);
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
            r2 = List1[i1].x;
            y = List1[i1].y;
            x = xfrm[0][0] * r2 + xfrm[0][1] * y + xfrm[0][2];
            y = xfrm[1][0] * r2 + xfrm[1][1] * y + xfrm[1][2];
            x -= List2[i2].x;
            y -= List2[i2].y;
            r2 = x * x + y * y;
            for (j=i; j>0 && r2<a.at<float>(1,j-1); j--)
                a.at<float>(1,j) = a.at<float>(1,j-1);
            a.at<float>(0,i) = r2;
            a.at<float>(1,j) = r2;
            sum += r2;
        }

        /* Set clipping limit and quit when no points are clipped. */
        i = 0.6 * m;
        r2 = CLIP * a.at<float>(1,i);
        if (r2 >= a.at<float>(1,m-1)) {
            rms = sqrt(sum / m);
            break;
        }

        Q_UNUSED(rms);

        /* Clip outliers and redo the fit. */
        j = 0;
        for (i=0; i<m; i++) {
            if (a.at<float>(0,i) < r2) {
                i1 = matches[0][i];
                i2 = matches[1][i];
                matches[0][j] = i1;
                matches[1][j] = i2;
                a.at<float>(0,j) = List1[i1].x;
                a.at<float>(1,j) = List1[i1].y;
                a.at<float>(2,j) = 1.;
                b.at<float>(0,j) = List2[i2].x;
                b.at<float>(1,j) = List2[i2].y;
                j++;
            }
        }
        m = j;

        if (m < MIN_MATCH) {
            if (ok)
                *ok = -1;
            return xfrm;
        }

        hfti(a, m, b, tau, krank, rnorm, h, g,ip);

        for (i=0; i<2; i++)
            for (j=0; j<3; j++)
                xfrm[i][j] = b[i][j];
    }

    return xfrm;
}



void openskystacker::hfti(cv::Mat a, int m, cv::Mat b, float tau, int &krank, float rnorm[], float h[], float g[], int ip[])
{
    float hmax = 0.0f;
    float factor = 0.001f;
    int lmax;

    int mda = a.cols;
    int n = a.rows;

    int mdb = b.cols;
    int nb = a.rows;

    int k = 0;
    int ldiag = std::min(m,n);

    if (ldiag <= 0) {
        krank = k;
        return;
    }

    for (int j = 0; j < ldiag; j++) {
        if (j != 0) {
            // update squared column lengths and find lmax
            lmax = j;
            for (int l=j; l < n; l++) {
                  h[l] -= pow(a.at<float>(j-1,l), 2);
                  if (h[l] > h[lmax]) lmax = l;
            }
        }

        if (hmax + factor*h[lmax] <= hmax) {
            // compute squared column lengths and find lmax
            lmax = j;
            for (int l=j; l <= n; l++) {
                h[l] = 0.0;

                for (int i=j; i <= m; i++) {
                    h[l] += pow(a.at<float>(i,l), 2);
                }

                if (h[l] > h[lmax]) lmax = l;
            }

            hmax = h[lmax];
        }

        // lmax has been determined
        // do column interchanges if needed.
        ip[j] = lmax;

        if (ip[j] != j) {
            for (int i = 0; i < m; i++) {
                float tmp = a.at<float>(i,j);
                a.at<float>(i,j) = a.at<float>(i,lmax);
                a.at<float>(i,lmax) = tmp;
            }
            h[lmax] = h[j];

        }

        // compute the j-th transformation and apply it to a and b
        h12(1, j, j+1, m, a(cv::Rect(0,j,0,a.cols-1)), &h[j], a(cv::Rect(0,j+1,0,a.cols-1)), 1, mda, n-j);
        h12(2, j, j+1, m, a(cv::Rect(0,j,0,a.cols-1)), &h[j], b, 1, mdb, nb);
    }

    // determine the pseudorank, k, using the tolerance, tau.
    int j;
    for (j = 0; j < ldiag; j++) {
        if (fabs(a.at<float>(j,j)) > tau) {
            break;
        }
    }
    if (fabs(a.at<float>(j,j)) <= tau)
        k = j - 1;
    else
        k = ldiag;

    int kp1 = k + 1;

    // compute the norms of the residual vectors.
    if (nb > 0) {
        for (int jb = 0; jb < nb; jb++) {
            float tmp = 0.0f;
            if (kp1 <= m) {
                for (int i = kp1; i < m; i++) {
                    tmp += pow(b.at<float>(i,jb),2);
                }
                rnorm[jb] = sqrt(tmp);
            }
        }
    }

    // special for pseudorank = 0
    if (k <= 0) {
        if (nb <= 0) {
            krank = k;
            return;
        }
        for (int jb = 0; jb < nb; jb++) {
            for (int i = 0; i < n; i++) {
                b.at<float>(i,jb) = 0.0f;
            }
        }
        krank = k;
        return;
    }

    // if the pseudorank is less than n, compute householder
    // decomposition of first k rows.
    if (k != n) {
        for (int ii = 0; ii < k; ii++) {
            int i = kp1 - ii;
            // TODO: THESE NUMBERS ARE STILL 1-INDEXED
            h12(1, i, kp1, n, a(cv::Rect(i,0,i,a.cols-1)), &g[i], a, mda, 1, i-1);
        }
    }

    if (nb <= 0) {
        krank = k;
        return;
    }

    for (int jb = 0; jb < nb; jb++) {
        // solve the k by k triangular system.
        for (int l = 0; l < k; l++) {
            double sm = 0.0;
            int i = kp1 - l;
            if (i != k) {
                int ip1 = i+1;
                for (int j = ip1; j < k; j++) {
                    sm += a.at<float>(i,j) * b.at<float>(j,jb);
                }
            }
            int sm1 = m;
            b.at<float>(i,jb) = (b.at<float>(i,jb) - sm1) / a.at<float>(i,i);
        }

        // complete computation of solution vector
        if (k != n) {
            for (int j = kp1; j < n; j++)
                b.at<float>(j,jb) = 0.0f;
            for (int i = 0; i < k; i++)
                h12(2, i, kp1, n, a(cv::Rect(0,i,0,a.cols-1)), &g[i], b(cv::Rect(0,jb,0,b.cols-1)), 1, mdb, 1);
        }

        // re-order the solution vector to compensate for the
        // column interchanges.
        for (int jj = 0; jj < ldiag; jj++) {
            int j = ldiag + 1 - jj;
            if (ip[j] == j)
                break;
            int l = ip[j];
            float tmp = b.at<float>(l,jb);
            b.at<float>(l,jb) = b.at<float>(j,jb);
            b.at<float>(j,jb) = tmp;
        }
    }

    // the solution vectors, x, are now
    // in the first  n  rows of the array b(,).
    krank = k;
}

void openskystacker::h12(int mode, int lpivot, int l1, int m, cv::Mat u, float *up, cv::Mat c, int ice, int icv, int ncv)
{
    float sm, b;

    if (lpivot <= 0 || lpivot >= l1 || l1 >= m ) return;
    float cl = fabs(u.at<float>(0,lpivot));

    if (mode == 1) {
        for (int j = l1; j <= m; j++) {
            if (u.at<float>(0,j)) cl = u.at<float>(0,j);
        }

        if (cl <= 0) return;

        float clinv = 1./cl;

        sm = pow(u.at<float>(0,lpivot) * clinv, 2);

        for (int j = l1; j <= m; j++) {
            sm += pow(u.at<float>(0,j) * clinv, 2);
        }

        float sm1 = sm;
        cl = cl * sqrt(sm1);

        if (u.at<float>(0,lpivot) > 0) cl = -cl;
        *up = u.at<float>(0,lpivot) - cl;
        u.at<float>(0,lpivot) = cl;
    }

    if (mode == 2 && cl <= 0) return;
    if (ncv <= 0) return;

    b = *up * u.at<float>(0,lpivot);
    if (b >= 0) return;

    b = 1./b;
    float i2 = 1 - icv + ice*(lpivot - 1);
    float incr = ice * (l1 - lpivot);

    for (int j = 1; j <= ncv; j++) {
        i2 += icv;
        float i3 = i2 + incr;
        float i4 = i3;

        sm=c[i2] * *up;
        for (int i = l1; i <= m; i++) {
            sm += c[i3] * u.at<float>(0,i);
            i3 += ice;
        }
        if (sm == 0) return;
        sm *= b;
        c[i2] = c[i2] + sm * *up;
        for (int i=l1; i <= m; i++) {
            c[i4] = c[i4] + sm * u.at<float>(0,i);
            i4=i4+ice;
        }
    }
}

