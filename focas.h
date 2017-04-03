#ifndef FOCAS_H
#define FOCAS_H

#include "opencv2/core.hpp"
#include <cmath>
#include <algorithm>
#include "triangle.h"
#include "star.h"
#include <qdebug.h>

std::vector<Triangle> generateTriangleList(std::vector<Star> List);
int sidesPos(int i, int j, int n);

void findMatches(int nobjs, std::vector<Triangle> List_triangA, std::vector<Triangle> List_triangB,
                 std::vector<Star> List1, std::vector<Star> List2);
void sortTriangles(std::vector<Triangle> *List_Triang_, int l, int r);
void binSearchTriangles(float key, std::vector<Triangle> *List_triang_, int *first, int *last);
void checkTolerance(int nobjs, Triangle List_triangA, std::vector<Triangle> *List_triangB_,
                    int first, int last, int Table_match[]);

#endif // FOCAS_H
