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


// Algorithm HFTI(a,m,n,b,τ,x,k,h,g,p)
void openskystacker::hfti(cv::Mat a, cv::Mat b, float tau, cv::Mat x, int &krank, cv::Mat h, cv::Mat g, cv::Mat p)
{
    // These values are inferred from the parameters
    int m = a.rows;
    int n = a.cols;

    float hmax = 0.0f;
    int lambda;

    krank = -1;

    // 1. Set μ := min(m,n).
    int ldiag = std::min(m,n);

    // 2. For j := 1 ,..., μ, do Steps 3-12.
    for (int j = 0; j < ldiag; j++) {

        // 3. If j = 1 go to Step 7.
        if (j != 0) {

            // 4. For l := j, ..., n, set h(l) := h(l) - a(j-1,l)^2.
            for (int l = j; l < n; l++) {
                h.at<float>(l) -= pow(a.at<float>(j-1, l), 2);
            }

            // 5. Determine λ such that h(λ) := max{h(l): j <= l <= n}.
            float maxh = 0.f;
            for (int l = j; l < n; l++) {
                if (h.at<float>(l) > hmax) {
                    maxh = h.at<float>(l);
                    lambda = l;
                }
            }
        }

        // 6. If (hmax + (10^-3)h(λ)) > hmax, go to Step 9.
        if (j == 0 || (hmax + .001f*h.at<float>(lambda)) <= hmax) {

            // 7. For l := j, ..., n, set h(l) := sum(a(i,l)^2), j <= i <= m
            for (int l = j; l < n; l++) {
                float sum = 0.f;
                for (int i = j; i < m; i++) {
                    float ail = a.at<float>(i,l);
                    sum += ail * ail;
                }
                h.at<float>(l) = sum;
            }

            // 8. Determine λ such that h(λ) := max{h(l): j <= l <= n}. Set hmax := h(λ)
            for (int l = j; l < n; l++) {
                if (h.at<float>(l) > hmax) {
                    hmax = h.at<float>(l);
                    lambda = l;
                }
            }
        }

        // 9. Set p(j) := λ. If p(j) = j, go to Step 11.
        p.at<int>(j) = lamdba;

        // 10. Interchange columns j and λ of A and set h(λ) := h(j)
        if (lambda != j) {
            interchangeCols(a, j, lambda);

            h.at<float>(lambda) = h.at<float>(j);
        }

        // 11. Execute algorithm H1(j, j+1, m, a(1,j), h(j), a(1,j+1), n-j).
        householder(1, j, j+1, a(cv::Rect(j,0,a.cols-j,1)), h.at<float>(j),
                    a(cv::Rect(j+1,0,a.cols-j-1-n-j,a.rows)));
        // 12. Execute algorithm H1(j, j+1, m, a(1,j), h(j), b, 1).
        householder(2, j, j+1, a(cv::Rect(j,0,a.cols-j,1)), h.at<float>(j),
                    b(cv::Rect(0,0,1,b.rows)));

    }

    // 13. The pseudorank k must now be determined. Note that the diagonal
    //     elements of R (stored in a(j,j) through a(μ,μ)) are non-
    //     increasing in magnitude. For example, the Fortran subroutine
    //     HFTI chooses k as the largest index j such that |a(j,j)| > τ.
    //     If all |a(j,j)| <= τ, the pseudorank k is set to zero, the
    //     solution vector x is set to zero, and the algorithm is
    //     terminated.
    krank = -1;
    for (int j = 0; j < ldiag; j++) {
        if (abs(a.at<float>(j,j)) > tau)
            krank = j;
    }
    if (krank < 0) {
        krank = 0;
        for (int j = 0; j < x.cols; j++) {
            for (int i = 0; i < x.rows; i++) {
                x.at<float>(i,j) = 0.f;
            }
        }
        return;
    }

    // 14. If k = n, go to Step 17.
    // 15. Here, k < n. Next, determine the orthogonal transformations Ki.
    if (krank != n) {

        // 16. For i := k, k-1, ..., 1, execute algorithm H1(i, k+1, n,
        //     a(i,1), g(i), a(1,1), i-1). (The parameters a(i,1) and
        //     a(1,1) each identify the first element of a row vector that
        //     is to be referenced.
        for (int i = krank; i >= 0; i--) {
            // TODO: 4th and 6th arguments are row vectors
            householder(1, i, krank+1, a(cv::Rect(0, i, 1, a.rows-i)), g.at<float>(i), a(cv::Rect(0,0,i-1,a.rows)), true);
        }
    }

    // 17. Set x(k) := b(k) / a(k,k). If k <= 1, go to Step 19.
    for (int i = 0; i < x.rows; i++) {
        x.at<float>(i,krank) = b.at<float>(i,krank) / a.at<float>(krank,krank);
    }


    if (krank > 1) {
        // 18. For i := k-1, k-2, ..., 1, x(i) := (b(i) - sum(a(i,j)x(j))/a(i,i), i+1 <= j <= k.
        for (int i = krank-1; i >= 0; i++) {
            for (int row = 0; row < x.rows; row++) {
                float sum = 0.f;
                for (int j = i+1; j <= krank; j++) {
                    sum += a.at<float>(i,j) * x.at<float>(j,row);
                }
                x.at<float>(i,row) = (b.at<float>(i,row) - sum) / a.at<float>(i,i);
            }
        }
    }

    // 19. If k = n, go to Step 22.
    if (krank != n) {
        // 20. For minimal length solution, set x(i) := 0, i := k+1, ..., n.
        for (int i = krank+1; i < n; i++) {
            x.at<float>(i) = 0;
        }

        // 21. For i := 1, ..., k, execute Algorithm H2(i, k+1, n, a(i,1),
        //     g(i), x, 1. Here a(i,1) identifies the first element of a
        //     row vector.
        for (int i = 0; i <= krank; i++) {
            // TODO: 4th argument is row vectors
            householder(2, i, krank+1, a(cv::Rect(0,i,1,a.rows-i)), g.at<float>(i), x);
        }
    }

    // 22. For j := μ, μ - 1, ..., 1, do Step 23.
    // 23. If p(j) != j, interchange the contents of x(j) and x(p(j)).
    for (int j = ldiag-1; j >= 0; j--) {
        if (p.at<int>(j) != j)
            interchangeCols(x, j, p(j));
    }
}

// Algorithms H1(p,l,m,u,h,c,v) [use steps 1-11] and H2(p,l,m,u,h,c,v) [use steps 5-11]
void openskystacker::householder(int mode, int p, int l, cv::Mat u, float &h, cv::Mat c, bool cHasRowVectors) {
    // These values are inferred from the parameters
    int m = max(u.rows, u.cols);
    int v = c.cols;

    float &up = u.at<float>(p);

    if (mode == 1) {
        // 1. Set s := sqrt(u(p)^2 + sum(u(i)^2)), l <= i <= m.
        float sum = 0;
        for (int i = l; i < m; i++) {
            float ui = u.at<float>(i);
            sum += ui * ui;
        }
        float s = sqrt(up * up + sum);

        // 2. If u(p) > 0, set s := -s.
        if (up > 0)
            s = -s;

        // 3. Set h := u(p) - s, u(p) := s
        h = up - s;
        up = s;
    }

    // 4. The construction of the transformation is complete. At Step 5 the
    //    application of the transformation to the vectors c(j) begins.

    // 5. b := u(p)h
    float b = up * h;

    // 6. If b = 0 or v = 0, go to Step 11.
    if (b != 0.f && v != 0) {
        // 7. For j := 1, ..., v, do Steps 8-10.
        for (int j = 0; j < v; j++) {
            // 8. Set s := (c(p,j) + sum(c(i,j)u(i))) / b, l <= i <= m.
            float sum = 0;
            for (int i = l; i < m; i++) {
                if (cHasRowVectors) {
                    sum += c.at<float>(j,i) * u.at<float>(i);
                } else {
                    sum += c.at<float>(i,j) * u.at<float>(i);
                }
            }
            float s;
            if (cHasRowVectors) {
                s = (c.at<float>(j,p) * h + sum) / b;
                c.at<float>(j,p) += s * h;
            } else {
                s = (c.at<float>(p,j) * h + sum) / b;
                // 9. Set c(p,j) := c(p,j) + sh.
                c.at<float>(p,j) += s * h;
            }

            // 10. For i := l, ..., m, set c(i,j) := c(i,j) + su(i)
            for (int i = l; i < m; i++) {
                if (cHasRowVectors) {
                    c.at<float>(j,i) += s * u.at<float>(i);
                } else {
                    c.at<float>(i,j) += s * u.at<float>(i);
                }
            }
        }
    }

    // 11. Algorithm H1 or H2 is completed.
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


void interchangeCols(cv::Mat mat, int col1, int col2)
{
    cv::Mat temp = a.col(col1).clone();
    a.col(col2).copyTo(a.col(col1));
    temp.copyTo(a.col(col2));
}
