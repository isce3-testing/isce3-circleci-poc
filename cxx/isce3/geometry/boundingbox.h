//-*- C++ -*-
//-*- coding: utf-8 -*-
//
// Author: Piyush Agram
// Copyright 2019

#pragma once

#include <isce3/core/LUT2d.h>

// isce3::product
#include <isce3/product/RadarGridParameters.h>

// isce3::geometry
#include "DEMInterpolator.h"
#include "Shapes.h"

// Declaration
namespace isce3 { namespace geometry {

/** Compute the perimeter of a radar grid in map coordinates.
 *
 * @param[in] radarGrid    RadarGridParameters object
 * @param[in] orbit         Orbit object
 * @param[in] proj          ProjectionBase object indicating desired projection
 * of output.
 * @param[in] doppler       LUT2d doppler model
 * @param[in] demInterp     DEM Interpolator
 * @param[in] pointsPerEge  Number of points to use on each edge of radar grid
 * @param[in] threshold     Slant range threshold for convergence
 * @param[in] numiter       Max number of iterations for convergence
 *
 * The outputs of this method is an OGRLinearRing.
 * Transformer from radar geometry coordinates to map coordinates with a DEM
 * The sequence of walking the perimeter is always in the following order :
 * <ul>
 * <li> Start at Early Time, Near Range edge. Always the first point of the
 * polygon. <li> From there, Walk along the Early Time edge to Early Time, Far
 * Range. <li> From there, walk along the Far Range edge to Late Time, Far
 * Range. <li> From there, walk along the Late Time edge to Late Time, Near
 * Range. <li> From there, walk along the Near Range edge back to Early Time,
 * Near Range.
 * </ul>
 */
Perimeter getGeoPerimeter(const isce3::product::RadarGridParameters& radarGrid,
        const isce3::core::Orbit& orbit,
        const isce3::core::ProjectionBase* proj,
        const isce3::core::LUT2d<double>& doppler = {},
        const DEMInterpolator& demInterp = DEMInterpolator(0.),
        const int pointsPerEdge = 11, const double threshold = 1.0e-8,
        const int numiter = 15);

/** Compute bounding box using min/ max altitude for quick estimates
 *
 * @param[in] radarGrid    RadarGridParameters object
 * @param[in] orbit         Orbit object
 * @param[in] proj          ProjectionBase object indicating desired projection
 * of output.
 * @param[in] doppler       LUT2d doppler model
 * @param[in] hgts          Vector of heights to use for the scene
 * @param[in] margin        Marging to add to estimated bounding box in decimal
 * degrees
 * @param[in] pointsPerEge  Number of points to use on each edge of radar grid
 * @param[in] threshold     Slant range threshold for convergence
 * @param[in] numiter       Max number of iterations for convergence
 *
 * The output of this method is an OGREnvelope.
 */
BoundingBox getGeoBoundingBox(
        const isce3::product::RadarGridParameters& radarGrid,
        const isce3::core::Orbit& orbit,
        const isce3::core::ProjectionBase* proj,
        const isce3::core::LUT2d<double>& doppler = {},
        const std::vector<double>& hgts = {isce3::core::GLOBAL_MIN_HEIGHT,
                isce3::core::GLOBAL_MAX_HEIGHT},
        const double margin = 0.0, const int pointsPerEdge = 11,
        const double threshold = 1.0e-8, const int numiter = 15,
        bool ignore_out_of_range_exception = false);

/** Compute bounding box with auto search within given min/ max height
 * interval
 *
 * @param[in] radarGrid    RadarGridParameters object
 * @param[in] orbit         Orbit object
 * @param[in] proj          ProjectionBase object indicating desired
 * projection of output.
 * @param[in] doppler       LUT2d doppler model
 * @param[in] minHeight     Height lower bound
 * @param[in] maxHeight     Height upper bound
 * @param[in] margin        Margin to add to estimated bounding box in
 * decimal degrees
 * @param[in] pointsPerEge  Number of points to use on each edge of radar
 * grid
 * @param[in] threshold     Slant range threshold for convergence
 * @param[in] numiter       Max number of iterations for convergence
 * @param[in] height_threshold Height threshold for convergence
 * The output of this method is an OGREnvelope.
 */
BoundingBox getGeoBoundingBoxHeightSearch(
        const isce3::product::RadarGridParameters& radarGrid,
        const isce3::core::Orbit& orbit,
        const isce3::core::ProjectionBase* proj,
        const isce3::core::LUT2d<double>& doppler = {},
        double min_height = isce3::core::GLOBAL_MIN_HEIGHT,
        double max_height = isce3::core::GLOBAL_MAX_HEIGHT,
        const double margin = 0.0, const int pointsPerEdge = 11,
        const double threshold = 1.0e-8, const int numiter = 15,
        const double height_threshold = 100);
}} // namespace isce3::geometry
// end of file
