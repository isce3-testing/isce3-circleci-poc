//-*- coding: utf-8 -*-
//
// Author: Bryan V. Riel
// Copyright: 2017-2018

#pragma once

#include <isce3/core/forward.h>

#include <valarray>

namespace isce3 { namespace cuda { namespace geometry {
// C++ interface for running geo2rdr for a block of data on GPU
void runGPUGeo2rdr(const isce3::core::Ellipsoid& ellipsoid,
        const isce3::core::Orbit& orbit,
        const isce3::core::LUT1d<double>& doppler,
        const std::valarray<double>& x, const std::valarray<double>& y,
        const std::valarray<double>& hgt, std::valarray<float>& azoff,
        std::valarray<float>& rgoff, int topoEPSG, size_t lineStart,
        size_t blockWidth, double t0, double r0, size_t length, size_t width,
        double prf, double rangePixelSpacing, double wavelength,
        isce3::core::LookSide side, double threshold, double numiter,
        unsigned int& totalconv);
}}} // namespace isce3::cuda::geometry
