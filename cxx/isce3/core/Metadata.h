// -*- C++ -*-
// -*- coding: utf-8 -*-
//
// Source Author: Bryan Riel
// Co-Author: Joshua Cohen
// Copyright 2017-2018

#pragma once

#include "forward.h"

#include <iosfwd>

#include "DateTime.h"
#include "LookSide.h"

/** Data structure for storing basic radar geometry image metadata */
class isce3::core::Metadata {
public:
    // Acquisition related parameters
    double radarWavelength;
    double prf;
    double rangeFirstSample;
    double slantRangePixelSpacing;
    double chirpSlope;
    double pulseDuration;
    double antennaLength;
    LookSide lookSide;
    DateTime sensingStart;
    double pegHeading, pegLatitude, pegLongitude;

    // Image formation related parametesr
    int numberRangeLooks;
    int numberAzimuthLooks;

    // Geometry parameters
    int width;
    int length;
};

// Define std::cout interaction for debugging
std::ostream& operator<<(std::ostream& os, const isce3::core::Metadata& radar);
