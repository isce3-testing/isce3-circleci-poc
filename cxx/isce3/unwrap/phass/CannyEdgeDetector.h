// Copyright (c) 2017-, California Institute of Technology ("Caltech"). U.S.
// Government sponsorship acknowledged.
// All rights reserved.
//
// Author(s):
//
//  ----------------------------------------------------------------------------
//  Author:  Xiaoqing Wu

#pragma once

#include <complex>
#include <list>
#include <queue>
#include <set>
#include <vector>

#include "DataPatch.h"
#include "Point.h"
#include "constants.h"

using namespace std;

class CannyEdgeDetector {
    // DataPatch<float> *data_patch;
    // DataPatch<float> *edge_patch;
    float** edge;
    void calculate(float** data);

public:
    int nr_lines;
    int nr_pixels;
    float no_data;

    float low_th;
    float high_th;

    int gw_size;     // gaussian window size default 7;
    double gw_sigma; // default 1

    void basic_init();
    ~CannyEdgeDetector();
    CannyEdgeDetector(int nr_lines, int nr_pixels, float no_data, float** data,
            float low, float high, int gw, double gw_sigma);

    float** get_edge() { return edge; }
    //  float **get_data() { return data_patch->get_data_lines_ptr(); }
};
