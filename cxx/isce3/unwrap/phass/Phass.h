// -*- C++ -*-
// -*- coding: utf-8 -*-
//
// Author: Heresh Fattahi
// Copyright 2019-

#pragma once

#include <complex> // std::complex
#include <cstddef> // size_t
#include <cstdint> // uint8_t

#include <isce3/io/Raster.h> // isce3::io::Raster

#include "PhassUnwrapper.h"

namespace isce3::unwrap::phass {

class Phass {
public:
    /** Constructor */
    Phass()
    {
        _correlationThreshold = 0.2;
        _goodCorrelation = 0.7;
        _minPixelsPerRegion = 200.0;
        _usePower = true;
    };

    /** Destructor */
    ~Phass() = default;

    /** Unwrap the interferometric wrapped phase. */
    void unwrap(isce3::io::Raster& phaseRaster, isce3::io::Raster& corrRaster,
            isce3::io::Raster& unwRaster, isce3::io::Raster& labelRaster);

    /** Unwrap the interferometric wrapped phase. */
    void unwrap(isce3::io::Raster& phaseRaster, isce3::io::Raster& powerRaster,
            isce3::io::Raster& corrRaster, isce3::io::Raster& unwRaster,
            isce3::io::Raster& labelRaster);

    /** Get correlation threshold increment. */
    double correlationThreshold() const;

    /** Set correlation threshold increment. */
    void correlationThreshold(const double);

    /** Get good correlation threshold. */
    double goodCorrelation() const;

    /** Set good correlation threshold. */
    void goodCorrelation(const double);

    /** Get minimum size of a region to be unwrapped. */
    int minPixelsPerRegion() const;

    /** Set minimum size of a region to be unwrapped. */
    void minPixelsPerRegion(const int);

private:
    double _correlationThreshold = 0.2;
    double _goodCorrelation = 0.7;
    int _minPixelsPerRegion = 200.0;
    bool _usePower = true;
};

} // namespace isce3::unwrap::phass

// Get inline implementations.
#define ISCE_UNWRAP_PHASS_PHASS_ICC
#include "Phass.icc"
#undef ISCE_UNWRAP_PHASS_PHASS_ICC
