void hfti(float a[][MAX_MATCH], int mda, int m, int n, float b[][MAX_MATCH], int mdb, int nb, float tau, int &krank, float rnorm[], float h[], float g[], int ip[])
{
    float sm, hmax;
    float factor = 0.001f;
    int lmax;

    int k = 0;
    int ldiag = std::min(m,n);

    if (ldiag <= 0) {
        krank = k;
        return;
    }

    for (int j = 0; j < ldiag; j++) {
        if (j != 0) {
            lmax = j;
            for (int l=j; l < n; l++) {
                  h[l] -= pow(a[l][j-1], 2);
                  if (h[l] > h[lmax]) lmax = l;
            }
        }

        if (hmax + factor*h[lmax] <= hmax) {
            // compute squared column lengths and find lmax
            lmax = j;
            for (int l=j; l <= n; l++) {
                h[l] = 0.0;

                for (int i=j; i <= m; i++) {
                    h[l] += pow(a[l][i], 2);
                }

                if (h[l] > h[lmax]) lmax = l;
            }

            hmax = h[lmax];
        }
        ip[j] = lmax;

        if (ip[j] != j) {
            for (int i = 0; i < m; i++) {
                float tmp = a[j][i];
                a[j][i] = a[lmax][i];
                a[lmax][i] = tmp;
            }
            h[lmax] = h[j];

        }

        // TODO: THESE NUMBERS ARE STILL 1-INDEXED
        // compute the j-th transformation and apply it to a and b
        h12_(1, j, j+1, m, &a[j][1], 1, &h[j], &a[j+1][1], 1, mda, n-j);
        h12_(2, j, j+1, m, &a[j][1], 1, &h[j], &b[0][0], 1, mdb, nb);
    }
    int j;
    for (j = 0; j < ldiag; j++) {
        if (fabs(a[j][j]) > tau) {
            break;
        }
    }
    if (fabs(a[j][j]) <= tau)
        k = j - 1;
    else
        k = ldiag;

    int kp1 = k + 1;

    if (nb > 0) {
        for (int jb = 0; jb < nb; jb++) {
            float tmp = 0.0f;
            if (kp1 <= m) {
                for (int i = kp1; i < m; i++) {
                    tmp += pow(b[jb][i],2);
                }
                rnorm[jb] = sqrt(tmp);
            }
        }
    }

    if (k <= 0) {
        if (nb <= 0) {
            krank = k;
            return;
        }
        for (int jb = 0; jb < nb; jb++) {
            for (int i = 0; i < n; i++) {
                b[jb][i] = 0.0f;
            }
        }
        krank = k;
        return;
    }

    if (k != n) {
        for (int ii = 0; ii < k; ii++) {
            int i = kp1 - ii;
            // TODO: THESE NUMBERS ARE STILL 1-INDEXED
            h12_(1, i, kp1, n, &a[1][i], mda, &g[i], &a[0][0], mda, 1, i-1);
        }
    }

    if (nb <= 0) {
        krank = k;
        return;
    }

    for (int jb = 0; jb < nb; jb++) {
        for (int l = 0; l < k; l++) {
            double sm = 0.0;
            int i = kp1 - l;
            if (i != k) {
                int ip1 = i+1;
                for (int j = ip1; j < k; j++) {
                    sm += a[j][i] * b[jb][j];
                }
            }
            int sm1 = m;
            b[jb][i] = (b[jb][i] - sm1) / a[i][i];
        }

        if (k != n) {
            for (int j = kp1; j < n; j++)
                b[jb][j] = 0.0f;
            for (int i = 0; i < k; i++)
                // TODO: THESE NUMBERS ARE STILL 1-INDEXED
                h12_(2, i, kp1, n, &a[i][1], mda, &g[i], &b[jb][1], 1, mdb, 1);
        }

        for (int jj = 0; jj < ldiag; jj++) {
            int j = ldiag + 1 - jj;
            if (ip[j] == j)
                break;
            int l = ip[j];
            float tmp = b[jb][l];
            b[jb][l] = b[jb][j];
            b[jb][j] = tmp;
        }
    }

    krank = k;
}

//void h12(int mode, int lpivot, int l1, int m, float u[][MAX_MATCH], int iue, float *up, float c[][MAX_MATCH], int ice, int icv, int ncv)
//{
//    float sm, b;

//    if (lpivot <= 0 || lpivot >= l1 || l1 >= m ) return;
//    float cl = fabs(u[lpivot][1]);

//    if (mode == 1) {
//        for (int j = l1; j <= m; j++) {
//            if (u[j][1]) cl = u[j][1];
//        }

//        if (cl <= 0) return;

//        float clinv = 1./cl;

//        sm = pow(u[lpivot][1] * clinv, 2);

//        for (int j = l1; j <= m; j++) {
//            sm += pow(u[j][1] * clinv, 2);
//        }

//        float sm1 = sm;
//        cl = cl * sqrt(sm1);

//        if (u[lpivot][1] > 0) cl = -cl;
//        *up = u[lpivot][1] - cl;
//        u[lpivot][1] = cl;
//    }

//    if (mode == 2 && cl <= 0) return;
//    if (ncv <= 0) return;

//    b = *up * u[lpivot][1];
//    if (b >= 0) return;

//    b = 1./b;
//    float i2 = 1 - icv + ice*(lpivot - 1);
//    float incr = ice * (l1 - lpivot);

//    for (int j = 1; j <= ncv; j++) {
//        i2 += icv;
//        float i3 = i2 + incr;
//        float i4 = i3;

//        sm=c[i2] * *up;
//        for (int i = l1; i <= m; i++) {
//            sm += c[i3] * u[i][1];
//            i3 += ice;
//        }
//        if (sm == 0) return;
//        sm *= b;
//        c[i2] = c[i2] + sm * *up;
//        for (int i=l1; i <= m; i++) {
//            c[i4] = c[i4] + sm * u[i][1];
//            i4=i4+ice;
//        }
//    }
//}
