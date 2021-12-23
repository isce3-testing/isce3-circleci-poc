//-*- C++ -*-
//-*- coding: utf-8 -*-
//
// Author: Bryan Riel
// Copyright 2017-2018

#include "Interpolator.h"

/** @param[in] x X-coordinate to interpolate
 * @param[in] y Y-coordinate to interpolate
 * @param[in] z 2D matrix to interpolate. */
template<class U>
U isce3::core::NearestNeighborInterpolator<U>::interp_impl(
        double x, double y, const Map& z) const
{

    // Nearest indices
    const auto row = static_cast<Eigen::Index>(std::round(y));
    const auto col = static_cast<Eigen::Index>(std::round(x));

    // No bounds check yet
    return z(row, col);
}

// Forward declaration of classes
template class isce3::core::NearestNeighborInterpolator<double>;
template class isce3::core::NearestNeighborInterpolator<float>;
template class isce3::core::NearestNeighborInterpolator<std::complex<double>>;
template class isce3::core::NearestNeighborInterpolator<std::complex<float>>;

// end of file
