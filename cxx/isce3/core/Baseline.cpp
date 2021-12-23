//
// Source Author: Bryan Riel
// Co-Author: Joshua Cohen
// Copyright 2017
//

#include "Baseline.h"

#include <cmath>

#include "Peg.h"
#include "Pegtrans.h"

namespace isce3 { namespace core {

void Baseline::init()
{
    // Initialize basis for the first orbit using the middle of the orbit
    double tmid = _orbit1.midTime();
    initBasis(tmid);

    // Use radar metadata to compute look vector at midpoint
    calculateLookVector(tmid);
}

void Baseline::initBasis(double t)
{
    // Interpolate orbit to azimuth time
    Vec3 xyz, vel;
    _orbit1.interpolate(&xyz, &vel, t, OrbitInterpBorderMode::FillNaN);
    _refxyz = xyz;
    _velocityMagnitude = vel.norm();

    // Get normalized vectors
    const Vec3 vel_norm = vel / _velocityMagnitude;
    _rhat = xyz.normalized();

    // Compute cross-track vectors
    _chat = _rhat.cross(vel_norm).normalized();

    // Compute velocity vector perpendicular to cross-track vector
    _vhat = _chat.cross(_rhat).normalized();
}

Vec3 Baseline::calculateBasisOffset(const Vec3& position) const
{
    const Vec3 dx = position - _refxyz;
    return {dx.dot(_vhat), dx.dot(_rhat), dx.dot(_chat)};
}

void Baseline::computeBaselines()
{
    Vec3 xyz2, vel2, offset;
    double t, delta_t;

    // Start with sensing mid of orbit 2
    t = _orbit2.midTime();
    for (int iter = 0; iter < 2; ++iter) {

        // Interpolate orbit to azimuth time
        _orbit2.interpolate(&xyz2, &vel2, t, OrbitInterpBorderMode::FillNaN);

        // Compute adjustment to slave time
        offset = calculateBasisOffset(xyz2);
        delta_t = offset[0] / _velocityMagnitude;
        t -= delta_t;
    }
    _bh = offset[2];
    _bv = offset[1];
}

void Baseline::calculateLookVector(double t)
{
    // Local working vectors
    Vec3 xyz, vel, llh;

    // Interpolate orbit to azimuth time
    _orbit1.interpolate(&xyz, &vel, t, OrbitInterpBorderMode::FillNaN);
    _elp.xyzToLonLat(xyz, llh);

    // Make a Peg
    Peg peg(llh[1], llh[0], _radar.pegHeading);

    // And a Peg Transformation
    Pegtrans ptm;
    ptm.radarToXYZ(_elp, peg);
    double Ra = ptm.radcur;
    double height = llh[2];
    double R0 = _radar.rangeFirstSample;
    _coslook = (height * ((2. * Ra) + height) + (R0 * R0)) /
               (2. * R0 * (Ra + height));
    _sinlook = sqrt(1. - (_coslook * _coslook));
}

}} // namespace isce3::core
