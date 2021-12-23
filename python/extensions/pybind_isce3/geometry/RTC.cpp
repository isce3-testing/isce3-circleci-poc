#include "RTC.h"

#include <limits>

#include <isce3/core/LUT2d.h>
#include <isce3/core/Orbit.h>
#include <isce3/io/Raster.h>
#include <isce3/product/RadarGridParameters.h>

namespace py = pybind11;

using isce3::geometry::rtcAlgorithm;
using isce3::geometry::rtcInputTerrainRadiometry;
using isce3::geometry::rtcOutputTerrainRadiometry;

void addbinding(py::enum_<rtcInputTerrainRadiometry>& pyInputTerrainRadiometry)
{
    pyInputTerrainRadiometry
            .value("BETA_NAUGHT", rtcInputTerrainRadiometry::BETA_NAUGHT)
            .value("SIGMA_NAUGHT_ELLIPSOID",
                    rtcInputTerrainRadiometry::SIGMA_NAUGHT_ELLIPSOID);
}

void addbinding(
        py::enum_<rtcOutputTerrainRadiometry>& pyOutputTerrainRadiometry)
{
    pyOutputTerrainRadiometry
            .value("SIGMA_NAUGHT", rtcOutputTerrainRadiometry::SIGMA_NAUGHT)
            .value("GAMMA_NAUGHT", rtcOutputTerrainRadiometry::GAMMA_NAUGHT);
}

void addbinding(py::enum_<rtcAlgorithm>& pyAlgorithm)
{
    pyAlgorithm
            .value("RTC_BILINEAR_DISTRIBUTION",
                    rtcAlgorithm::RTC_BILINEAR_DISTRIBUTION)
            .value("RTC_AREA_PROJECTION", rtcAlgorithm::RTC_AREA_PROJECTION);
}

void addbinding(py::enum_<rtcAreaMode>& pyAreaMode)
{
    pyAreaMode.value("AREA", rtcAreaMode::AREA)
            .value("AREA_FACTOR", rtcAreaMode::AREA_FACTOR);
}

void addbinding_apply_rtc(pybind11::module& m)
{
    m.def("apply_rtc", &isce3::geometry::applyRtc, py::arg("radar_grid"),
            py::arg("orbit"), py::arg("input_dop"), py::arg("input_raster"),
            py::arg("dem_raster"), py::arg("output_raster"),
            py::arg("input_terrain_radiometry") =
                    rtcInputTerrainRadiometry::BETA_NAUGHT,
            py::arg("output_terrain_radiometry") =
                    rtcOutputTerrainRadiometry::GAMMA_NAUGHT,
            py::arg("exponent") = 0,
            py::arg("rtc_area_mode") =
                    isce3::geometry::rtcAreaMode::AREA_FACTOR,
            py::arg("rtc_algorithm") = rtcAlgorithm::RTC_AREA_PROJECTION,
            py::arg("geogrid_upsampling") =
                    std::numeric_limits<double>::quiet_NaN(),
            py::arg("rtc_min_value_db") =
                    std::numeric_limits<float>::quiet_NaN(),
            py::arg("abs_cal_factor") = 1,
            py::arg("clip_min") = std::numeric_limits<float>::quiet_NaN(),
            py::arg("clip_max") = std::numeric_limits<float>::quiet_NaN(),
            py::arg("radar_grid_nlooks") = 1, py::arg("out_nlooks") = nullptr,
            py::arg("input_rtc") = nullptr, py::arg("output_rtc") = nullptr,
            py::arg("rtc_memory_mode") =
                    isce3::core::MemoryModeBlockY::AutoBlocksY,
            R"(This function computes and applies the radiometric terrain correction (RTC) to a multi-band
              raster.

              Parameters
              ---------
              radar_grid : isce3.product.RadarGridParameters
                  Radar Grid
              orbit : isce3.core.Orbit
                  Orbit
              input_dop : isce3.core.LUT2d
                  Doppler LUT
              input_raster : isce3.io.Raster
                  Input raster
              dem_raster : isce3.io.Raster
                  Input DEM raster
              output_raster : isce3.io.Raster
                  Output raster (output)
              input_terrain_radiometry : isce3.geometry.RtcInputTerrainRadiometry, optional
                  Input terrain radiometry
              output_terrain_radiometry : isce3.geometry.RtcOutputTerrainRadiometry, optional
                  Output terrain radiometry
              exponent : int, optional
                  Exponent to be applied to the input data. The
               value 0 indicates that the the exponent is based on the
               data type of the input raster (1 for real and 2 for complex
               rasters).
              rtc_area_mode : isce3.geometry.RtcAreaMode, optional
                  RTC area mode
              rtc_algorithm : isce3.geometry.RtcAlgorithm, optional
                  RTC algorithm
              geogrid_upsampling : double, optional
                  Geogrid upsampling (in each direction)
              rtc_min_value_db : float, optional
                  Minimum value for the RTC area factor. Radar data with RTC
               area factor below this limit are ignored.
              abs_cal_factor : double, optional
                  Absolute calibration factor.
              clip_min : float, optional
                  Clip minimum output values
              clip_max : float, optional
                  Clip maximum output values
              radar_grid_nlooks : float, optional
                  Radar grid number of looks. This parameters determines
               the multilooking factor used to compute out_nlooks.
              out_nlooks : isce3.io.Raster, optional
                  Raster to which the number of radar-grid
               looks associated with the geogrid will be saved (output)
              input_rtc : isce3.io.Raster, optional
                  Raster containing pre-computed RTC area factor
              output_rtc : isce3.io.Raster, optional
                  Output RTC area factor (output)
              rtc_memory_mode : isce3.core.memory_mode_block_y, optional
                  Select memory mode
              )");
}

void addbinding_compute_rtc(pybind11::module& m)
{
    m.def("compute_rtc",
            py::overload_cast<const isce3::product::RadarGridParameters&,
                    const isce3::core::Orbit&,
                    const isce3::core::LUT2d<double>&, isce3::io::Raster&,
                    isce3::io::Raster&, rtcInputTerrainRadiometry,
                    rtcOutputTerrainRadiometry, isce3::geometry::rtcAreaMode,
                    rtcAlgorithm, double, float, float, isce3::io::Raster*,
                    isce3::core::MemoryModeBlockY,
                    isce3::core::dataInterpMethod, double, int, double>(
                    &isce3::geometry::computeRtc),
            py::arg("radar_grid"), py::arg("orbit"), py::arg("input_dop"),
            py::arg("dem"), py::arg("output_raster"),
            py::arg("input_terrain_radiometry") =
                    rtcInputTerrainRadiometry::BETA_NAUGHT,
            py::arg("output_terrain_radiometry") =
                    rtcOutputTerrainRadiometry::GAMMA_NAUGHT,
            py::arg("rtc_area_mode") =
                    isce3::geometry::rtcAreaMode::AREA_FACTOR,
            py::arg("rtc_algorithm") = rtcAlgorithm::RTC_AREA_PROJECTION,
            py::arg("geogrid_upsampling") =
                    std::numeric_limits<double>::quiet_NaN(),
            py::arg("rtc_min_value_db") =
                    std::numeric_limits<float>::quiet_NaN(),
            py::arg("radar_grid_nlooks") = 1, py::arg("out_nlooks") = nullptr,
            py::arg("rtc_memory_mode") =
                    isce3::core::MemoryModeBlockY::AutoBlocksY,
            py::arg("interp_method") =
                    isce3::core::dataInterpMethod::BIQUINTIC_METHOD,
            py::arg("threshold") = 1e-4, py::arg("num_iter") = 100,
            py::arg("delta_range") = 1e-4,
            R"(This function computes and applies the radiometric terrain correction
             (RTC) to a multi-band raster.

             Parameters
             ----------
             radar_grid : isce3.product.RadarGridParameters
                 Radar Grid
             orbit : isce3.core.Orbit
                 Orbit
             input_dop : isce3.core.LUT2d
                 Doppler LUT
             dem_raster : isce3.io.Raster
                 Input DEM raster
             output_raster : isce3.io.Raster
                 Output raster (output)
             input_terrain_radiometry : isce3.geometry.RtcInputTerrainRadiometry, optional
                 Terrain radiometry of the input raster
             output_terrain_radiometry : isce3.geometry.RtcOutputTerrainRadiometry, optional
                 Terrain radiometry of the input raster
             rtc_area_mode : isce3.geometry.RtcAreaMode, optional
                 RTC area mode
             rtc_algorithm : isce3.geometry.RtcAlgorithm, optional
                 RTC algorithm
             geogrid_upsampling : double, optional
                Geogrid upsampling (in each direction)
             rtc_min_value_db : float, optional
                 Minimum value for the RTC area factor. Radar
              data with RTC area factor below this limit are ignored.
             radar_grid_nlooks : float, optional
                 Radar grid number of looks. This
              parameters determines the multilooking factor used to compute out_nlooks.
             out_nlooks : isce3.io.Raster, optional
                 Raster to which the number of radar-grid
              looks associated with the geogrid will be saved (output)
             rtc_memory_mode : isce3.core.memory_mode_block_y, optional
                 Select memory mode
             interp_method : isce3.core.DataInterpMethod, optional
                 Interpolation Method
             threshold : double, optional
                 Distance threshold for convergence
             num_iter : int, optional
                 Maximum number of Newton-Raphson iterations
             delta_range : double, optional
                Step size used for computing Doppler derivative
             )");
}

void addbinding_compute_rtc_bbox(pybind11::module& m)
{
    m.def("compute_rtc_bbox",
            py::overload_cast<isce3::io::Raster&, isce3::io::Raster&,
                    const isce3::product::RadarGridParameters&,
                    const isce3::core::Orbit&,
                    const isce3::core::LUT2d<double>&, const double,
                    const double, const double, const double, const int,
                    const int, const int, rtcInputTerrainRadiometry,
                    rtcOutputTerrainRadiometry, isce3::geometry::rtcAreaMode,
                    rtcAlgorithm, double, float, float, isce3::io::Raster*,
                    isce3::io::Raster*, isce3::io::Raster*,
                    isce3::core::MemoryModeBlockY,
                    isce3::core::dataInterpMethod, double, int, double>(
                    &isce3::geometry::computeRtc),
            py::arg("dem_raster"), py::arg("output_raster"),
            py::arg("radar_grid"), py::arg("orbit"), py::arg("input_dop"),
            py::arg("y0"), py::arg("dy"), py::arg("x0"), py::arg("dx"),
            py::arg("geogrid_length"), py::arg("geogrid_width"),
            py::arg("epsg"),
            py::arg("input_terrain_radiometry") =
                    rtcInputTerrainRadiometry::BETA_NAUGHT,
            py::arg("output_terrain_radiometry") =
                    rtcOutputTerrainRadiometry::GAMMA_NAUGHT,
            py::arg("rtc_area_mode") =
                    isce3::geometry::rtcAreaMode::AREA_FACTOR,
            py::arg("rtc_algorithm") = rtcAlgorithm::RTC_AREA_PROJECTION,
            py::arg("geogrid_upsampling") =
                    std::numeric_limits<double>::quiet_NaN(),
            py::arg("rtc_min_value_db") =
                    std::numeric_limits<float>::quiet_NaN(),
            py::arg("radar_grid_nlooks") = 1, py::arg("out_geo_rdr") = nullptr,
            py::arg("out_geo_grid") = nullptr, py::arg("out_nlooks") = nullptr,
            py::arg("rtc_memory_mode") =
                    isce3::core::MemoryModeBlockY::AutoBlocksY,
            py::arg("interp_method") =
                    isce3::core::dataInterpMethod::BIQUINTIC_METHOD,
            py::arg("threshold") = 1e-4, py::arg("num_iter") = 100,
            py::arg("delta_range") = 1e-4,
            R"(This function computes and applies the radiometric terrain correction
             (RTC) to a multi-band raster using a predefined geogrid.

              Parameters
             ----------
             dem_raster : isce3.io.Raster
                 Input DEM raster
             output_raster : isce3.io.Raster
                 Output raster (output)
             radar_grid : isce3.product.RadarGridParameters
                 Radar Grid
             orbit : isce3.core.Orbit
                 Orbit
             input_dop : isce3.core.LUT2d
                 Doppler LUT
             y0 : double
                 Starting northing position
             dy : double
                 Northing step size
             x0 : double
                 Starting easting position
             dx : double
                 Easting step size
             geogrid_length : int
                 Geographic length (number of pixels) in the northing direction
             geogrid_width : int
                 Geographic width (number of pixels) in the easting direction
             epsg : int
                 Output geographic grid EPSG
             input_terrain_radiometry : isce3.geometry.RtcInputTerrainRadiometry, optional
                 Terrain radiometry of the input raster
             output_terrain_radiometry : isce3.geometry.RtcOutputTerrainRadiometry, optional
                 Terrain radiometry of the input raster
             rtc_area_mode : isce3.geometry.RtcAreaMode, optional
                 RTC area mode
             rtc_algorithm : isce3.geometry.RtcAlgorithm, optional
                 RTC algorithm
             geogrid_upsampling : double, optional
                 Geogrid upsampling (in each direction)
             rtc_min_value_db : float, optional
                 Minimum value for the RTC area factor. Radar
              data with RTC area factor below this limit are ignored.
             radar_grid_nlooks : float, optional
                 Radar grid number of looks. This
              parameters determines the multilooking factor used to compute out_nlooks.
             out_geo_rdr : isce3.io.Raster, optional
                 Raster to which the radar-grid positions
              (range and azimuth) of the geogrid pixels vertices will be saved (output).
             out_geo_grid : isce3.io.Raster, optional
                 Raster to which the radar-grid positions
              (range and azimuth) of the geogrid pixels center will be saved (output).
             out_nlooks : isce3.io.Raster, optional
                 Raster to which the number of radar-grid
              looks associated with the geogrid will be saved (output)
             rtc_memory_mode : isce3.core.memory_mode_block_y, optional
                 Select memory mode
             interp_method : isce3.core.DataInterpMethod, optional
                 Interpolation Method
             threshold : double, optional
                 Distance threshold for convergence
             num_iter : int, optional
                 Maximum number of Newton-Raphson iterations
             delta_range : double, optional
                 Step size used for computing Doppler derivative
             )");
}
