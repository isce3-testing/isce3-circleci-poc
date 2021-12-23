//
// Author: Joshua Cohen
// Copyright 2017
//

#include "Ellipsoid.h"

#include <cmath>

using isce3::core::Vec3;

/** @param[in] pos ECEF coordinates of imaging platform in meters
 *  @param[in] vel ECEF velocity of imaging platform in meters / sec
 *  @param[in] los Line-of-sight (LOS) vector in ECEF coordinates (meters),
 * pointing from platform to target
 *  @param[out] azi Azimuth angle in radians
 *  @param[out] look Look angle in radians
 *
 *  Azimuth angle is defined as angle of the LOS vector from the North Direction
 * in the anti-clockwise direction. Look angle is defined as angle of the LOS
 * vector and the downward normal at the imaging platform .
 */
void isce3::core::Ellipsoid::getImagingAnglesAtPlatform(const cartesian_t& pos,
        const cartesian_t& vel, const cartesian_t& los, double& azi,
        double& look) const
{
    /*
     * Computes the look vector given the look angle, azimuth angle, and
     * position vector
     */
    // Estimate lat,lon for platform position
    const Vec3 llh = xyzToLonLat(pos);

    // Estimate normal to ellipsoid at platform position
    const Vec3 n = nVector(llh[0], llh[1]);

    // Angle subtended with n-vector pointing downwards
    look = std::acos((-n).dot(los) / los.norm());

    // Get Cross
    const Vec3 c = n.cross(vel).normalized();
    const Vec3 t = c.cross(n).normalized();
    azi = std::atan2(c.dot(los), t.dot(los));
}
