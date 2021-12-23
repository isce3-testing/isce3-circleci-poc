//-*- coding: utf-8 -*_
//
// Author: Bryan V. Riel
// Copyright: 2017-2018

#pragma once

#include "forward.h"
#include <isce3/geometry/forward.h>

#include <isce3/geometry/Topo.h>

// CUDA Topo class definition
/** Transformer from radar geometry coordinates to map coordinates with DEM /
 * reference altitude on GPU
 *
 * See <a href="overview_geometry.html#forwardgeom">geometry overview</a> for a
 * description of the algorithm*/
class isce3::cuda::geometry::Topo : public isce3::geometry::Topo {

public:
    /** Constructor from Product */
    inline Topo(const isce3::product::Product& product, char frequency = 'A',
            bool nativeDoppler = false)
        : isce3::geometry::Topo(product, frequency, nativeDoppler)
    {}

    inline Topo(const isce3::product::RadarGridParameters& radarGrid,
            const isce3::core::Orbit& orbit,
            const isce3::core::Ellipsoid& ellipsoid,
            const isce3::core::LUT2d<double>& doppler = {})
        : isce3::geometry::Topo(radarGrid, orbit, ellipsoid, doppler)
    {}

    /** Constructor from isce3::core objects */
    inline Topo(const isce3::core::Ellipsoid& ellps,
            const isce3::core::Orbit& orbit,
            const isce3::core::LUT2d<double>& doppler,
            const isce3::core::Metadata& meta)
        : isce3::geometry::Topo(ellps, orbit, doppler, meta)
    {}

    /** Run topo - main entrypoint; internal creation of topo rasters */
    void topo(isce3::io::Raster&, const std::string&);

    /** Run topo with externally created topo rasters */
    void topo(isce3::io::Raster& demRaster, isce3::io::Raster& xRaster,
            isce3::io::Raster& yRaster, isce3::io::Raster& heightRaster,
            isce3::io::Raster& incRaster, isce3::io::Raster& hdgRaster,
            isce3::io::Raster& localIncRaster,
            isce3::io::Raster& localPsiRaster, isce3::io::Raster& simRaster);

    /** Run topo with externally created topo rasters (plus mask raster) */
    void topo(isce3::io::Raster& demRaster, isce3::io::Raster& xRaster,
            isce3::io::Raster& yRaster, isce3::io::Raster& heightRaster,
            isce3::io::Raster& incRaster, isce3::io::Raster& hdgRaster,
            isce3::io::Raster& localIncRaster,
            isce3::io::Raster& localPsiRaster, isce3::io::Raster& simRaster,
            isce3::io::Raster& maskRaster);

    /** Run topo - main entrypoint; internal creation of topo rasters */
    void topo(isce3::io::Raster&, isce3::geometry::TopoLayers&);

private:
    // Default number of lines per block
    size_t _linesPerBlock = 1000;

    // Compute number of lines per block dynamically from GPU memmory
    void computeLinesPerBlock(isce3::io::Raster&, isce3::geometry::TopoLayers&);

    // Generate layover/shadow masks using an orbit
    void _setLayoverShadowWithOrbit(const isce3::core::Orbit& orbit,
            isce3::geometry::TopoLayers& layers,
            isce3::geometry::DEMInterpolator& demInterp, size_t lineStart);
};
