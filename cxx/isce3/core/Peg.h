//
// Author: Joshua Cohen
// Copyright 2017
//

#pragma once

#include "forward.h"

/** Data structure to store a peg point
 *
 * Peg points are used for SCH coordinate system representation for UAVSAR*/
class isce3::core::Peg {
public:
    // peg latitude in radians
    double lat;

    // peg longiture in radians
    double lon;

    // peg heading in radians
    double hdg;

    /** Simple peg point constructor
     *
     * @param[in] lt Latitude in radians
     * @param[in] ln Longitude in radians
     * @param[in] hd Heading in radians*/
    Peg(double lt, double ln, double hd) : lat(lt), lon(ln), hdg(hd) {}

    /** Empty constructor */
    Peg() : Peg(0., 0., 0.) {}

    /** Copy constructor */
    Peg(const Peg& p) : lat(p.lat), lon(p.lon), hdg(p.hdg) {}

    /** Assignment operator */
    inline Peg& operator=(const Peg&);
};

isce3::core::Peg& isce3::core::Peg::operator=(const Peg& rhs)
{
    lat = rhs.lat;
    lon = rhs.lon;
    hdg = rhs.hdg;
    return *this;
}
