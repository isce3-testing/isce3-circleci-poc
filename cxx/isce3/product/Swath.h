//-*- C++ -*-
//-*- coding: utf-8 -*-

#pragma once

// std
#include <valarray>

// isce3::core
#include <isce3/core/Constants.h>
#include <isce3/core/DateTime.h>
#include <isce3/core/LUT2d.h>

// isce3::io
#include <isce3/io/Raster.h>

// Declaration
namespace isce3 { namespace product {
class Swath;
}} // namespace isce3::product

// isce3::product::Swath definition
class isce3::product::Swath {

public:
    // Constructors
    inline Swath() {};

    /** Get slant range array */
    inline const std::valarray<double>& slantRange() const
    {
        return _slantRange;
    }
    /** Set slant range array */
    inline void slantRange(const std::valarray<double>& rng)
    {
        _slantRange = rng;
    }

    /** Get the range pixel spacing */
    inline double rangePixelSpacing() const
    {
        return _slantRange[1] - _slantRange[0];
    }

    /** Get zero Doppler time array */
    inline const std::valarray<double>& zeroDopplerTime() const
    {
        return _zeroDopplerTime;
    }
    /** Set zero Doppler time array */
    inline void zeroDopplerTime(const std::valarray<double>& t)
    {
        _zeroDopplerTime = t;
    }

    /** Get the number of samples */
    inline size_t samples() const { return _slantRange.size(); }

    /** Get the number of lines */
    inline size_t lines() const { return _zeroDopplerTime.size(); }

    /** Get acquired center frequency */
    inline double acquiredCenterFrequency() const
    {
        return _acquiredCenterFrequency;
    }
    /** Set acquired center frequency */
    inline void acquiredCenterFrequency(double f)
    {
        _acquiredCenterFrequency = f;
    }

    /** Get processed center frequency */
    inline double processedCenterFrequency() const
    {
        return _processedCenterFrequency;
    }
    /** Set processed center frequency */
    inline void processedCenterFrequency(double f)
    {
        _processedCenterFrequency = f;
    }

    /** Get processed wavelength */
    inline double processedWavelength() const
    {
        return isce3::core::speed_of_light / _processedCenterFrequency;
    }

    /** Get acquired range bandwidth */
    inline double acquiredRangeBandwidth() const
    {
        return _acquiredRangeBandwidth;
    }
    /** Set acquired range bandwidth */
    inline void acquiredRangeBandwidth(double b)
    {
        _acquiredRangeBandwidth = b;
    }

    /** Get processed range bandwidth */
    inline double processedRangeBandwidth() const
    {
        return _processedRangeBandwidth;
    }
    /** Set acquired range bandwidth */
    inline void processedRangeBandwidth(double b)
    {
        _processedRangeBandwidth = b;
    }

    /** Get nominal acquisition PRF */
    inline double nominalAcquisitionPRF() const
    {
        return _nominalAcquisitionPRF;
    }
    /** Set nominal acquisition PRF */
    inline void nominalAcquisitionPRF(double f) { _nominalAcquisitionPRF = f; }

    /** Get time spacing of raster grid */
    inline double zeroDopplerTimeSpacing() const
    {
        return _zeroDopplerTimeSpacing;
    }
    /** Set time spacing of raster grid */
    inline void zeroDopplerTimeSpacing(double dt)
    {
        _zeroDopplerTimeSpacing = dt;
    }

    /** Get scene center along track spacing */
    inline double sceneCenterAlongTrackSpacing() const
    {
        return _sceneCenterAlongTrackSpacing;
    }
    /** Set scene center along track spacing */
    inline void sceneCenterAlongTrackSpacing(double s)
    {
        _sceneCenterAlongTrackSpacing = s;
    }

    /** Get scene center ground range spacing */
    inline double sceneCenterGroundRangeSpacing() const
    {
        return _sceneCenterGroundRangeSpacing;
    }
    /** Set scene center ground range spacing */
    inline void sceneCenterGroundRangeSpacing(double s)
    {
        _sceneCenterGroundRangeSpacing = s;
    }

    /** Get processed azimuth bandwidth */
    inline double processedAzimuthBandwidth() const
    {
        return _processedAzimuthBandwidth;
    }
    /** Set processed azimuth bandwidth */
    inline void processedAzimuthBandwidth(double b)
    {
        _processedAzimuthBandwidth = b;
    }

    /** Get reference epoch */
    inline const isce3::core::DateTime& refEpoch() const { return _refEpoch; }
    /** Set reference epoch */
    inline void refEpoch(const isce3::core::DateTime& epoch)
    {
        _refEpoch = epoch;
    }

    /** Get valid array indices */
    inline std::array<size_t, 2> validSamples() const
    {
        return {_validStart, _validEnd};
    }
    /** Set valid array indices */
    inline void validSamples(const std::array<size_t, 2>& valid)
    {
        _validStart = valid[0];
        _validEnd = valid[1];
    }

private:
    // Coordinates
    std::valarray<double> _slantRange;
    std::valarray<double> _zeroDopplerTime;

    // Other metadata
    double _acquiredCenterFrequency;
    double _processedCenterFrequency;
    double _acquiredRangeBandwidth;
    double _processedRangeBandwidth;
    double _nominalAcquisitionPRF;  // acquired
    double _zeroDopplerTimeSpacing; // processed
    double _sceneCenterAlongTrackSpacing;
    double _sceneCenterGroundRangeSpacing;
    double _processedAzimuthBandwidth;
    size_t _validStart;
    size_t _validEnd;

    // Reference epoch
    isce3::core::DateTime _refEpoch;
};
