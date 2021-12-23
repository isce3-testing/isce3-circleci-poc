#pragma once

#include "forward.h"

#include <string>
#include <valarray>

#include <isce3/io/Raster.h>

class isce3::geometry::TopoLayers {

public:
    // Default constructor
    TopoLayers() : _length(0.0), _width(0.0), _haveRasters(false) {}
    // Constructors
    TopoLayers(size_t length, size_t width)
        : _length(length), _width(width), _haveRasters(false)
    {
        _x.resize(length * width);
        _y.resize(length * width);
        _z.resize(length * width);
        _inc.resize(length * width);
        _hdg.resize(length * width);
        _localInc.resize(length * width);
        _localPsi.resize(length * width);
        _sim.resize(length * width);
        _mask.resize(length * width);
        _crossTrack.resize(length * width);
    }
    // Destructor
    ~TopoLayers()
    {
        if (_haveRasters) {
            delete _xRaster;
            delete _yRaster;
            delete _zRaster;
            delete _incRaster;
            delete _hdgRaster;
            delete _localIncRaster;
            delete _localPsiRaster;
            delete _simRaster;
            if (_maskRaster) {
                delete _maskRaster;
            }
        }
    }

    // Set new block sizes
    void setBlockSize(size_t length, size_t width)
    {
        _length = length;
        _width = width;
        _x.resize(length * width);
        _y.resize(length * width);
        _z.resize(length * width);
        _inc.resize(length * width);
        _hdg.resize(length * width);
        _localInc.resize(length * width);
        _localPsi.resize(length * width);
        _sim.resize(length * width);
        _mask.resize(length * width);
        _crossTrack.resize(length * width);
    }

    // Get sizes
    inline size_t length() const { return _length; }
    inline size_t width() const { return _width; }

    // Initialize rasters
    void initRasters(const std::string& outdir, size_t width, size_t length,
            bool computeMask = false)
    {

        // Initialize the standard output rasters
        _xRaster = new isce3::io::Raster(
                outdir + "/x.rdr", width, length, 1, GDT_Float64, "ISCE");
        _yRaster = new isce3::io::Raster(
                outdir + "/y.rdr", width, length, 1, GDT_Float64, "ISCE");
        _zRaster = new isce3::io::Raster(
                outdir + "/z.rdr", width, length, 1, GDT_Float64, "ISCE");
        _incRaster = new isce3::io::Raster(
                outdir + "/inc.rdr", width, length, 1, GDT_Float32, "ISCE");
        _hdgRaster = new isce3::io::Raster(
                outdir + "/hdg.rdr", width, length, 1, GDT_Float32, "ISCE");
        _localIncRaster = new isce3::io::Raster(outdir + "/localInc.rdr", width,
                length, 1, GDT_Float32, "ISCE");
        _localPsiRaster = new isce3::io::Raster(outdir + "/localPsi.rdr", width,
                length, 1, GDT_Float32, "ISCE");
        _simRaster = new isce3::io::Raster(
                outdir + "/simamp.rdr", width, length, 1, GDT_Float32, "ISCE");

        // Optional mask raster
        if (computeMask) {
            _maskRaster = new isce3::io::Raster(
                    outdir + "/mask.rdr", width, length, 1, GDT_Byte, "ISCE");
        } else {
            _maskRaster = nullptr;
        }

        // Update sizes
        _width = width;
        _length = length;

        // Indicate that we have initialized rasters
        _haveRasters = true;
    }

    // Set rasters (plus mask raster) from externally created rasters
    void setRasters(isce3::io::Raster& xRaster, isce3::io::Raster& yRaster,
            isce3::io::Raster& zRaster, isce3::io::Raster& incRaster,
            isce3::io::Raster& hdgRaster, isce3::io::Raster& localIncRaster,
            isce3::io::Raster& localPsiRaster, isce3::io::Raster& simRaster)
    {
        _xRaster = &xRaster;
        _yRaster = &yRaster;
        _zRaster = &zRaster;
        _incRaster = &incRaster;
        _hdgRaster = &hdgRaster;
        _localIncRaster = &localIncRaster;
        _localPsiRaster = &localPsiRaster;
        _simRaster = &simRaster;

        // Update sizes
        _width = xRaster.width();
        _length = xRaster.length();

        if ((yRaster.width() != _width) || (yRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the input y raster is different from x "
                    "raster "
                    "All input rasters must have the same shape.");
        }

        if ((zRaster.width() != _width) || (zRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the input z raster is different from x "
                    "raster "
                    "All input rasters must have the same shape.");
        }

        if ((incRaster.width() != _width) || (incRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the input incidence angle raster is "
                    "different from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((hdgRaster.width() != _width) || (hdgRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the input heading raster is different "
                    "from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((localIncRaster.width() != _width) ||
                (localIncRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the input local incidence angle raster "
                    "is different from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((localPsiRaster.width() != _width) ||
                (localPsiRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the input local Psi raster is different "
                    "from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((simRaster.width() != _width) || (simRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the input simulated amplitude raster is "
                    "different from x raster "
                    "All input rasters must have the same shape.");
        }
    }

    // Set rasters (plus mask raster) from externally created rasters
    void setRasters(isce3::io::Raster& xRaster, isce3::io::Raster& yRaster,
            isce3::io::Raster& zRaster, isce3::io::Raster& incRaster,
            isce3::io::Raster& hdgRaster, isce3::io::Raster& localIncRaster,
            isce3::io::Raster& localPsiRaster, isce3::io::Raster& simRaster,
            isce3::io::Raster& maskRaster)
    {
        _xRaster = &xRaster;
        _yRaster = &yRaster;
        _zRaster = &zRaster;
        _incRaster = &incRaster;
        _hdgRaster = &hdgRaster;
        _localIncRaster = &localIncRaster;
        _localPsiRaster = &localPsiRaster;
        _simRaster = &simRaster;
        _maskRaster = &maskRaster;

        // Update sizes
        _width = xRaster.width();
        _length = xRaster.length();

        if ((yRaster.width() != _width) || (yRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the y raster is different from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((zRaster.width() != _width) || (zRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the z raster is different from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((incRaster.width() != _width) || (incRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the incidence angle raster is different "
                    "from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((hdgRaster.width() != _width) || (hdgRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the heading raster is different from x "
                    "raster "
                    "All input rasters must have the same shape.");
        }

        if ((localIncRaster.width() != _width) ||
                (localIncRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the local incidence angle raster is "
                    "different from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((localPsiRaster.width() != _width) ||
                (localPsiRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the local Psi raster is different from x "
                    "raster "
                    "All input rasters must have the same shape.");
        }

        if ((simRaster.width() != _width) || (simRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the simulated amplitude raster is "
                    "different from x raster "
                    "All input rasters must have the same shape.");
        }

        if ((maskRaster.width() != _width) ||
                (maskRaster.length() != _length)) {
            throw isce3::except::DomainError(ISCE_SRCINFO(),
                    "The shape of the shadow/layover raster is different "
                    "from x raster "
                    "All input rasters must have the same shape.");
        }
    }

    // Get array references
    std::valarray<double>& x() { return _x; }
    std::valarray<double>& y() { return _y; }
    std::valarray<double>& z() { return _z; }
    std::valarray<float>& inc() { return _inc; }
    std::valarray<float>& hdg() { return _hdg; }
    std::valarray<float>& localInc() { return _localInc; }
    std::valarray<float>& localPsi() { return _localPsi; }
    std::valarray<float>& sim() { return _sim; }
    std::valarray<short>& mask() { return _mask; }
    std::valarray<double>& crossTrack() { return _crossTrack; }

    // Set values for a single index
    void x(size_t row, size_t col, double value)
    {
        _x[row * _width + col] = value;
    }

    void y(size_t row, size_t col, double value)
    {
        _y[row * _width + col] = value;
    }

    void z(size_t row, size_t col, double value)
    {
        _z[row * _width + col] = value;
    }

    void inc(size_t row, size_t col, float value)
    {
        _inc[row * _width + col] = value;
    }

    void hdg(size_t row, size_t col, float value)
    {
        _hdg[row * _width + col] = value;
    }

    void localInc(size_t row, size_t col, float value)
    {
        _localInc[row * _width + col] = value;
    }

    void localPsi(size_t row, size_t col, float value)
    {
        _localPsi[row * _width + col] = value;
    }

    void sim(size_t row, size_t col, float value)
    {
        _sim[row * _width + col] = value;
    }

    void mask(size_t row, size_t col, short value)
    {
        _mask[row * _width + col] = value;
    }

    void crossTrack(size_t row, size_t col, double value)
    {
        _crossTrack[row * _width + col] = value;
    }

    // Get values for a single index
    double x(size_t row, size_t col) const { return _x[row * _width + col]; }

    double y(size_t row, size_t col) const { return _y[row * _width + col]; }

    double z(size_t row, size_t col) const { return _z[row * _width + col]; }

    float inc(size_t row, size_t col) const { return _inc[row * _width + col]; }

    float hdg(size_t row, size_t col) const { return _hdg[row * _width + col]; }

    float localInc(size_t row, size_t col) const
    {
        return _localInc[row * _width + col];
    }

    float localPsi(size_t row, size_t col) const
    {
        return _localPsi[row * _width + col];
    }

    float sim(size_t row, size_t col) const { return _sim[row * _width + col]; }

    short mask(size_t row, size_t col) const
    {
        return _mask[row * _width + col];
    }

    double crossTrack(size_t row, size_t col) const
    {
        return _crossTrack[row * _width + col];
    }

    // Write data with rasters
    void writeData(size_t xidx, size_t yidx);

private:
    // The valarrays for the actual data
    std::valarray<double> _x;
    std::valarray<double> _y;
    std::valarray<double> _z;
    std::valarray<float> _inc;
    std::valarray<float> _hdg;
    std::valarray<float> _localInc;
    std::valarray<float> _localPsi;
    std::valarray<float> _sim;
    std::valarray<short> _mask;
    std::valarray<double>
            _crossTrack; // internal usage only; not saved to Raster

    // Raster pointers for each layer
    isce3::io::Raster* _xRaster;
    isce3::io::Raster* _yRaster;
    isce3::io::Raster* _zRaster;
    isce3::io::Raster* _incRaster;
    isce3::io::Raster* _hdgRaster;
    isce3::io::Raster* _localIncRaster;
    isce3::io::Raster* _localPsiRaster;
    isce3::io::Raster* _simRaster;
    isce3::io::Raster* _maskRaster;

    // Dimensions
    size_t _length, _width;

    // Directory for placing rasters
    std::string _topodir;

    // Flag indicates if the Class owns the Rasters
    // Should be True when initRasters is called
    // Should be false when the Rasters are passed
    // from outside and setRaster method is called
    bool _haveRasters;
};
