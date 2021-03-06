#include "GeocodeSlc.h"

#include <isce3/core/Ellipsoid.h>
#include <isce3/core/LUT2d.h>
#include <isce3/core/Orbit.h>
#include <isce3/core/Poly2d.h>
#include <isce3/geocode/geocodeSlc.h>
#include <isce3/io/Raster.h>
#include <isce3/product/RadarGridParameters.h>
#include <isce3/product/GeoGridParameters.h>

#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include <vector>

namespace py = pybind11;

template<typename AzRgFunc = isce3::core::Poly2d>
void addbinding_geocodeslc(py::module & m)
{
    m.def("geocode_slc", py::overload_cast<isce3::io::Raster &,
            isce3::io::Raster &, isce3::io::Raster &,
            const isce3::product::RadarGridParameters &,
            const isce3::product::GeoGridParameters &,
            const isce3::core::Orbit &,
            const isce3::core::LUT2d<double> &,
            const isce3::core::LUT2d<double> &,
            const isce3::core::Ellipsoid &,
            const double &, const int &,
            const size_t &,
            const bool,
            const AzRgFunc &,
            const AzRgFunc &,
            const std::complex<float>>(&isce3::geocode::geocodeSlc<AzRgFunc>),
        py::arg("output_raster"),
        py::arg("input_raster"),
        py::arg("dem_raster"),
        py::arg("radargrid"),
        py::arg("geogrid"),
        py::arg("orbit"),
        py::arg("native_doppler"),
        py::arg("image_grid_doppler"),
        py::arg("ellipsoid"),
        py::arg("threshold_geo2rdr") = 1.0e-9,
        py::arg("numiter_geo2rdr") = 25,
        py::arg("lines_per_block") = 1000,
        py::arg("flatten") = true,
        py::arg("azimuth_carrier") = AzRgFunc(),
        py::arg("range_carrier") = AzRgFunc(),
        py::arg("invalid_value") =
            std::complex<float>(std::numeric_limits<float>::quiet_NaN(),
                                std::numeric_limits<float>::quiet_NaN()),
        R"(
        Geocode a SLC

        Parameters
        ----------
        outputRaster: Raster
            Output raster containing geocoded SLC
        inputRaster: Raster
            Input raster of the SLC in radar coordinates
        demRaster: Raster
            Raster of the DEM
        radargrid: RadarGridParameters
            Radar grid parameters of input SLC raster
        geogrid: GeoGridParameters
            Geo grid parameters of output raster
        native_doppler: LUT2d
            2D LUT doppler of the SLC image
        image_grid_doppler: LUT2d
            2d LUT doppler of the image grid
        ellipsoid: Ellipsoid
            Ellipsoid object
        threshold_geo2rdr: float
            Threshold for geo2rdr computations
        numiter_geo2rdr: int
            Maximum number of iterations for geo2rdr convergence
        lines_per_block: int
            Number of lines per block
        flatten: bool
            Flag to flatten the geocoded SLC
        azimuth_carrier: [LUT2d, Poly2d]
            Azimuth carrier phase of the SLC data, in radians, as a function of azimuth and range
        range_carrier: [LUT2d, Poly2d]
            Range carrier phase of the SLC data, in radians, as a function of azimuth and range
        invalid_value: complex
            invalid pixel fill value
        )");
    m.def("geocode_slc", py::overload_cast<isce3::io::Raster &,
            isce3::io::Raster &, isce3::io::Raster &,
            const isce3::product::RadarGridParameters &,
            const isce3::product::RadarGridParameters &,
            const isce3::product::GeoGridParameters &,
            const isce3::core::Orbit &,
            const isce3::core::LUT2d<double> &,
            const isce3::core::LUT2d<double> &,
            const isce3::core::Ellipsoid &,
            const double &, const int &,
            const size_t &,
            const bool,
            const AzRgFunc &,
            const AzRgFunc &,
            const std::complex<float>>(&isce3::geocode::geocodeSlc<AzRgFunc>),
        py::arg("output_raster"),
        py::arg("input_raster"),
        py::arg("dem_raster"),
        py::arg("radargrid"),
        py::arg("sliced_radargrid"),
        py::arg("geogrid"),
        py::arg("orbit"),
        py::arg("native_doppler"),
        py::arg("image_grid_doppler"),
        py::arg("ellipsoid"),
        py::arg("threshold_geo2rdr") = 1.0e-9,
        py::arg("numiter_geo2rdr") = 25,
        py::arg("lines_per_block") = 1000,
        py::arg("flatten") = true,
        py::arg("azimuth_carrier") = AzRgFunc(),
        py::arg("range_carrier") = AzRgFunc(),
        py::arg("invalid_value") =
            std::complex<float>(std::numeric_limits<float>::quiet_NaN(),
                                std::numeric_limits<float>::quiet_NaN()),
        R"(
        Geocode a SLC

        Parameters
        ----------
        outputRaster: Raster
            Output raster containing geocoded SLC
        inputRaster: Raster
            Input raster of the SLC in radar coordinates
        demRaster: Raster
            Raster of the DEM
        radargrid: RadarGridParameters
            Radar grid parameters of input SLC raster
        sliced_radargrid: RadarGridParameters
            Radar grid representing subset of radargrid
        geogrid: GeoGridParameters
            Geo grid parameters of output raster
        native_doppler: LUT2d
            2D LUT doppler of the SLC image
        image_grid_doppler: LUT2d
            2d LUT doppler of the image grid
        ellipsoid: Ellipsoid
            Ellipsoid object
        threshold_geo2rdr: float
            Threshold for geo2rdr computations
        numiter_geo2rdr: int
            Maximum number of iterations for geo2rdr convergence
        lines_per_block: int
            Number of lines per block
        flatten: bool
            Flag to flatten the geocoded SLC
        azimuth_carrier: [LUT2d, Poly2d]
            Azimuth carrier phase of the SLC data, in radians, as a function of azimuth and range
        range_carrier: [LUT2d, Poly2d]
            Range carrier phase of the SLC data, in radians, as a function of azimuth and range
        invalid_value: complex
            invalid pixel fill value
        )");
}

template void addbinding_geocodeslc<isce3::core::LUT2d<double>>(py::module & m);
template void addbinding_geocodeslc<isce3::core::Poly2d>(py::module & m);
