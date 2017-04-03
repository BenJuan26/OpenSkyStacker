#ifndef HFTI_H
#define HFTI_H

#define MAX_MATCH	120	/* Max # of matches to use (~MAX_OBJS) */

// Fortran code is compiled and linked externally
extern "C" {
    extern void hfti_(float (*)[MAX_MATCH], int*, int*, int*, float (*)[MAX_MATCH], int*, int*, float*, int*, float*, float*, float*, int*);
}

#endif // HFTI_H
