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

class ChangeDetector {
    DataPatch<float>* data_patch;
    DataPatch<unsigned char>* change_patch;
    void calculate();

public:
    int nr_lines;
    int nr_pixels;
    float no_data;

    int change_type; // 0: ratio; 1: difference
    float change_th;

    int window_size; // default 3;
    int iterations;  // default 3
    int max_change;

    void basic_init();
    ~ChangeDetector();
    ChangeDetector(int nr_lines, int nr_pixels, float no_data,
            DataPatch<float>* data_patch, int change_type, float change_th,
            int window, int iter);

    unsigned char** get_change_data()
    {
        return change_patch->get_data_lines_ptr();
    }
    float** get_data() { return data_patch->get_data_lines_ptr(); }
};
