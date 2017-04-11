#include "focas.h"

std::vector<Triangle> generateTriangleList(std::vector<Star> List)
{
    int nobjs = NOBJS;
    if (List.size() < NOBJS) nobjs = List.size();
    qDebug() << "Using" << nobjs << "objects";

    //ntriang = (nobjs - 2) * (nobjs - 1) * nobjs / 6;
    std::vector<Triangle> List_triang; //(ntriang);

    int	i, j, k, p=0;
    float	di, dj, dk, a, b, c, h1, h2;

    std::vector<float> sides(nobjs * (nobjs - 1)/2 + 1);

    /* Minimize the number of triangle sides to compute */
    for (i = 0; i < nobjs; i++) {
        for (j = i + 1; j < nobjs; j++) {
            h1 = (List[i].getX() - List[j].getX());
            h2 = (List[i].getY() - List[j].getY());
            k = sidesPos(i, j, nobjs);
            sides[k] = sqrt(h1*h1 + h2*h2);
        }
    }

    /* Order the triangle sides and compute the triangle space coords */
    for (i = 0; i < (nobjs-2); i++) {
        for (j = i+1; j < (nobjs-1); j++) {
            for (k = j+1; k < nobjs; k++) {
                di = sides [sidesPos(i, j, nobjs)] ;
                dj = sides [sidesPos(j, k, nobjs)] ;
                dk = sides [sidesPos(k, i, nobjs)] ;

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

int sidesPos(int i, int j, int n)
{
    if (i < j)
        return(i*(2*n-i-3)/2 + j);
    else
        return(j*(2*n-j-3)/2 + i);
}

std::vector<std::vector<int> > findMatches(int nobjs, int *k_, std::vector<Triangle> List_triangA, std::vector<Triangle> List_triangB, std::vector<Star> List1, std::vector<Star> List2)
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
    sortTriangles(&List_triangB, 0, List_triangB.size() - 1);

    /* Find objects within tolerance distance in triangle space. */
    for (i = 0; i < List_triangA.size(); i++) {
        binSearchTriangles(List_triangA[i].x, &List_triangB, &first, &last);
        checkTolerance(nobjs, List_triangA[i], &List_triangB, first, last, Table_match);
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

    delete Table_match;

    return matches;
}

void sortTriangles(std::vector<Triangle> *List_Triang_, int l, int r)
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

        sortTriangles(&List_Triang, l, i-1);
        sortTriangles(&List_Triang, i+1, r);
    }
}

void binSearchTriangles(float key, std::vector<Triangle> *List_triang_, int *first, int *last)
{
    std::vector<Triangle> &List_triang = *List_triang_;
    int ntriang = List_triang.size();
    int	min = 0, max = ntriang - 1, middle ;
    int	found = 0, i;
    float tolerance = TOL;

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

void checkTolerance(int nobjs, Triangle List_triangA, std::vector<Triangle> *List_triangB_, int first, int last, int Table_match[])
{
    std::vector<Triangle> &List_triangB = *List_triangB_;
    float tolerance = TOL;
    float tolerance2 = TOL * TOL;
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

std::vector<std::vector<float> > findTransform(std::vector<std::vector<int> > matches, int m, std::vector<Star> List1, std::vector<Star> List2)
{
    //float	xfrm[2][3];
    std::vector<std::vector<float> > xfrm(2, std::vector<float>(3,0));

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
        a[0][i] = List1[matches[0][i]].getX();
        a[1][i] = List1[matches[0][i]].getY();
        a[2][i] = 1.;
        b[0][i] = List2[matches[1][i]].getX();
        b[1][i] = List2[matches[1][i]].getY();
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
        r2 = List1[i1].getX();
        y = List1[i1].getY();
        x = xfrm[0][0] * r2 + xfrm[0][1] * y + xfrm[0][2];
        y = xfrm[1][0] * r2 + xfrm[1][1] * y + xfrm[1][2];
        x -= List2[i2].getX();
        y -= List2[i2].getY();
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
            a[0][j] = List1[i1].getX();
            a[1][j] = List1[i1].getY();
            a[2][j] = 1.;
            b[0][j] = List2[i2].getX();
            b[1][j] = List2[i2].getY();
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

    return xfrm;
}
