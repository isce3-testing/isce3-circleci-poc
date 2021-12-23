// -*- C++ -*-
// -*- coding: utf-8 -*-
//
// Author: Bryan Riel
// Copyright 2017-2018

#pragma once

#include "forward.h"

// pyre
#include <pyre/journal.h>

// isce3::core
#include <isce3/core/forward.h>
#include <isce3/io/forward.h>

#include <isce3/core/Constants.h>
#include <isce3/core/Interpolator.h>
#include <isce3/error/ErrorCode.h>

// DEMInterpolator declaration
class isce3::geometry::DEMInterpolator {

    using cartesian_t = isce3::core::Vec3;

public:
    /** Default constructor with reference height of 0, bilinear interpolation
     */
    inline DEMInterpolator()
        : _haveRaster {false}, _refHeight {0.0}, _meanValue {0.0},
          _maxValue {0.0}, _interpMethod {isce3::core::BILINEAR_METHOD}
    {}

    /** Constructor with custom reference height and bilinear interpolation */
    inline DEMInterpolator(float height, int epsg = 4326)
        : _haveRaster {false}, _refHeight {height},
          _meanValue {height}, _maxValue {height}, _epsgcode {epsg},
          _interpMethod {isce3::core::BILINEAR_METHOD}
    {}

    /** Constructor with custom reference height and custom interpolator method
     */
    inline DEMInterpolator(
            float height, isce3::core::dataInterpMethod method, int epsg = 4326)
        : _haveRaster {false}, _refHeight {height}, _meanValue {height},
          _maxValue {height}, _epsgcode {epsg}, _interpMethod {method}
    {}

    /** Destructor */
    ~DEMInterpolator();

    /** Read in subset of data from a DEM with a supported projection */
    isce3::error::ErrorCode loadDEM(isce3::io::Raster& demRaster, double minX,
            double maxX, double minY, double maxY);

    /** Read in entire DEM with a supported projection */
    void loadDEM(isce3::io::Raster& demRaster);

    // Print stats
    void declare() const;

    /** Compute max and mean DEM height */
    void computeHeightStats(
            float& maxValue, float& meanValue, pyre::journal::info_t& info);

    /** Interpolate at a given longitude and latitude */
    double interpolateLonLat(double lon, double lat) const;
    /** Interpolate at native XY coordinates of DEM */
    double interpolateXY(double x, double y) const;

    /** Get starting X coordinate */
    double xStart() const { return _xstart; }
    /** Set starting X coordinate */
    void xStart(double xstart) { _xstart = xstart; }

    /** Get starting Y coordinate */
    double yStart() const { return _ystart; }
    /** Set starting Y coordinate */
    void yStart(double ystart) { _ystart = ystart; }

    /** Get X spacing */
    double deltaX() const { return _deltax; }
    /** Set X spacing */
    void deltaX(double deltax) { _deltax = deltax; }

    /** Get Y spacing */
    double deltaY() const { return _deltay; }
    /** Set Y spacing */
    void deltaY(double deltay) { _deltay = deltay; }

    /** Get mid X coordinate */
    double midX() const { return _xstart + 0.5 * width() * _deltax; }
    /** Get mid Y coordinate */
    double midY() const { return _ystart + 0.5 * length() * _deltay; }
    /** Get mid longitude and latitude */
    cartesian_t midLonLat() const;

    /** Flag indicating whether a DEM raster has been loaded */
    bool haveRaster() const { return _haveRaster; }

    /** Get reference height of interpolator */
    double refHeight() const { return _refHeight; }
    /** Set reference height of interpolator */
    void refHeight(double h) { _refHeight = h; }

    /** Get mean height value */
    inline double meanHeight() const { return _meanValue; }

    /** Get max height value */
    inline double maxHeight() const { return _maxValue; }

    /** Get pointer to underlying DEM data */
    float* data() { return _dem.data(); }

    /** Get pointer to underlying DEM data */
    const float* data() const { return _dem.data(); }

    /** Get width of DEM data used for interpolation */
    inline size_t width() const
    {
        return (_haveRaster ? _dem.width() : _width);
    }
    /** Set width of DEM data used for interpolation */
    inline void width(int width) { _width = width; }

    /** Get length of DEM data used for interpolation */
    inline size_t length() const
    {
        return (_haveRaster ? _dem.length() : _length);
    }
    /** Set length of DEM data used for interpolation */
    inline void length(int length) { _length = length; }

    /** Get EPSG code for input DEM */
    inline int epsgCode() const { return _epsgcode; }
    /** Set EPSG code for input DEM */
    void epsgCode(int epsgcode);

    /** Get Pointer to a ProjectionBase */
    inline isce3::core::ProjectionBase* proj() const { return _proj; }

    /** Get interpolator method enum */
    inline isce3::core::dataInterpMethod interpMethod() const
    {
        return _interpMethod;
    }
    /** Set interpolator method enum */
    inline void interpMethod(isce3::core::dataInterpMethod interpMethod)
    {
        _interpMethod = interpMethod;
    }

private:
    // Flag indicating whether we have access to a DEM raster
    bool _haveRaster;
    // Constant value if no raster is provided
    float _refHeight;
    // Statistics
    float _meanValue;
    float _maxValue;
    // Pointer to a ProjectionBase
    int _epsgcode;
    isce3::core::ProjectionBase* _proj = nullptr;
    // Pointer to an Interpolator
    isce3::core::dataInterpMethod _interpMethod;
    isce3::core::Interpolator<float>* _interp = nullptr;
    // 2D array for storing DEM subset
    isce3::core::Matrix<float> _dem;
    // Starting x/y for DEM subset and spacing
    double _xstart, _ystart, _deltax, _deltay;
    int _width, _length;
};
