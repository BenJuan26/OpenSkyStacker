/* This code was derived from the FOCAS program, whose copyright can be found below.    */
/*                                                                                      */
/* Copyright(c) 1982 Association of Universities for Research in Astronomy Inc.         */
/*                                                                                      */
/* The FOCAS software is publicly available, but is NOT in the public domain.           */
/* The difference is that copyrights granting rights for unrestricted use and           */
/* redistribution have been placed on all of the software to identify its authors.      */
/* You are allowed and encouraged to take this software and use it as you wish,         */
/* subject to the restrictions outlined below.                                          */
/*                                                                                      */
/* Permission to use, copy, modify, and distribute this software and its                */
/* documentation is hereby granted without fee, provided that the above                 */
/* copyright notice appear in all copies and that both that copyright notice            */
/* and this permission notice appear in supporting documentation, and that              */
/* references to the Association of Universities for Research in Astronomy              */
/* Inc. (AURA), the National Optical Astronomy Observatories (NOAO), or the             */
/* Faint Object Classification and Analysis System (FOCAS) not be used in               */
/* advertising or publicity pertaining to distribution of the software without          */
/* specific, written prior permission from NOAO.  NOAO makes no                         */
/* representations about the suitability of this software for any purpose.  It          */
/* is provided "as is" without express or implied warranty.                             */
/*                                                                                      */
/* NOAO DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL            */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NOAO            */
/* BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES          */
/* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION         */
/* OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN               */
/* CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.                             */

#ifndef FOCAS_H
#define FOCAS_H

#include "libstacker/model.h"
#include "hfti.h"

#include <qdebug.h>

#include <cmath>
#include <algorithm>

#include "opencv2/core/core.hpp"

namespace openskystacker {

/*! @file focas.h
    @brief Variables and functions from the FOCAS library.
*/

#define TOL		0.002   /*!< Default matching tolerance */
#define	NOBJS		40	/*!< Default number of objects */
#define MIN_MATCH	6	/*!< Min # of matched objects for transform */
#define MAX_MATCH	120	/*!< Max # of matches to use (~MAX_OBJS) */
#define MIN_OBJS	10	/*!< Min # of objects to use */
#define MAX_OBJS	100	/*!< Max # of objects to use */
#define	CLIP		3	/*!< Sigma clipping factor */
#define PM		57.2958 /*!< Radian to degree conversion */

//! Generates a list of triangles to be used for the alignment algorithm.
/*!
    @param List The list of stars to generate triangles from.
*/
std::vector<Triangle> GenerateTriangleList(std::vector<Star> List);
int SidesPos(int i, int j, int n);

std::vector<std::vector<int> > FindMatches(int nobjs, int *k, std::vector<Triangle> List_triangA, std::vector<Triangle> List_triangB);
std::vector<std::vector<float> > FindTransform(std::vector<std::vector<int> > matches,
        int m, std::vector<Star> List1, std::vector<Star> List2, int *ok = 0);
void SortTriangles(std::vector<Triangle> *List_Triang_, int l, int r);
void BinSearchTriangles(float key, std::vector<Triangle> *List_triang_, int *first, int *last);
void CheckTolerance(int nobjs, Triangle List_triangA, std::vector<Triangle> *List_triangB_,
                    int first, int last, int Table_match[]);
// void h12(int mode, int lpivot, int l1, int m, float u[][MAX_MATCH], int iue, float *up, float c[][MAX_MATCH], int ice, int icv, int ncv);
// void hfti(cv::Mat a, int mda, int m, int n, cv::Mat b, int mdb, int nb, float tau, int krank, float rnorm[], float h[], float g[], int ip[]);

}

#endif // FOCAS_H
