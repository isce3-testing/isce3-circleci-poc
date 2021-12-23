//-*- C++ -*-
//-*- coding: utf-8 -*-

#include <cmath>
#include <iostream>
#include <valarray>
#include <vector>

#include "gtest/gtest.h"

// isce3::core
#include "isce3/core/LUT1d.h"
#include "isce3/core/Utilities.h"
#include "isce3/cuda/core/gpuLUT1d.h"

// Reference values generated by scipy.interpolate.interp1d example
std::valarray<double> ref_vals {1.5669373788524215, 1.485946324730647,
        1.4049552706088724, 1.323964216487098, 1.2429731623653235,
        1.161982108243549, 1.0809910541217744, 1.0, 0.9190089458782256,
        0.8380178917564511, 0.7570268376346766, 0.6875149974964754,
        0.6294823713418476, 0.5714497451872198, 0.513417119032592,
        0.47183492535797783, 0.43025273168336364, 0.3886705380087495,
        0.3529819693063402, 0.3231870255761357, 0.2933920818459313,
        0.26359713811572677, 0.2422481280362511, 0.2208991179567754,
        0.1995501078772997, 0.18122698575171198, 0.16592975158001225,
        0.1506325174083125, 0.1353352832366127, 0.12437433598741056,
        0.11341338873820835, 0.1024524414890062, 0.09304503691560456,
        0.08519117501800354, 0.07733731312040257, 0.06948345122280154,
        0.06385591326424796, 0.05822837530569435, 0.05260083734714077,
        0.04697329938858716, 0.04134576143003357, 0.03571822347147996,
        0.030090685512926346, 0.024463147554372768, 0.018835609595819154,
        0.013208071637265575, 0.007580533678711955, 0.0019529957201583764,
        -0.003674542238395237, -0.00930208019694885};

TEST(LUT1dTest, Lookup)
{

    // Construct coordinates and values for LUT
    const size_t n = 10;
    std::valarray<double> coords(n), values(n);
    for (size_t i = 0; i < n; ++i) {
        coords[i] = i;
        values[i] = std::exp(-1.0 * i / 3.0);
    }

    // Create LUT with extrapolation beyond the coordinate bounds
    isce3::core::LUT1d<double> lut(coords, values, true);

    // Create GPU LUT
    isce3::cuda::core::gpuLUT1d<double> gpuLUT(lut);

    // Construct coordinates for evaluating LUT
    const size_t n_eval = 50;
    std::vector<double> xvec = isce3::core::linspace(-2.0, 12.0, n_eval);
    std::valarray<double> x_eval(xvec.data(), xvec.size());

    // Evaluate LUT
    for (size_t i = 0; i < n_eval; ++i) {
        double value = gpuLUT.eval_h(x_eval[i]);
        EXPECT_NEAR(value, ref_vals[i], 1.0e-13);
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// end of file
