#include "metadataCubes.h"

#include <isce3/core/Ellipsoid.h>
#include <isce3/core/LUT2d.h>
#include <isce3/core/Orbit.h>
#include <isce3/io/Raster.h>
#include <isce3/product/GeoGridParameters.h>
#include <isce3/product/RadarGridParameters.h>

namespace py = pybind11;

void addbinding_metadata_cubes(py::module& m)
{
    m.def("make_radar_grid_cubes", &isce3::geometry::makeRadarGridCubes,
            py::arg("radar_grid"), py::arg("geogrid"), py::arg("heights"),
            py::arg("orbit"), py::arg("native_doppler"),
            py::arg("grid_doppler"),
            py::arg("epsg_los_and_along_track_vectors") = 0,
            py::arg("slant_range_raster") = nullptr,
            py::arg("azimuth_time_raster") = nullptr,
            py::arg("incidence_angle_raster") = nullptr,
            py::arg("lo_unit_vector_x_raster") = nullptr,
            py::arg("los_unit_vector_y_raster") = nullptr,
            py::arg("along_track_unit_vector_x_raster") = nullptr,
            py::arg("along_track_unit_vector_y_raster") = nullptr,
            py::arg("elevation_angle_raster") = nullptr,
            py::arg("ground_track_velocity_raster") = nullptr,
            py::arg("threshold_geo2rdr") = 1.0e-8,
            py::arg("numiter_geo2rdr") = 100, py::arg("delta_range") = 1.0e-8,
            R"(Make metadata radar grid cubes

             Metadata radar grid cubes describe the radar geometry
             over a three-dimensional grid, defined by
             a reference geogrid and a vector of heights.

             The representation as cubes, rather than two-dimensional rasters,
             is intended to reduce the amount of disk space required to
             store radar geometry values within NISAR L2 products.

             This is possible because each cube contains slow-varying values
             in space, that can be described by a low-resolution
             three-dimensional grid with sufficient accuracy.

             These values, however, are usually required at the
             terrain height, often characterized by a fast-varying surface
             representing the local topography. A high-resolution DEM can
             then be used to interpolate the metadata cubes and generate
             high-resolution maps of the corresponding radar geometry variable.

             The line-of-sight (LOS) and along-track unit vectors are referenced to
             the projection defined by the epsg_los_and_along_track_vectors code.
             In the case of ENU, i.e. epsg_los_and_along_track_vectors equals to 0
             or 4326, ENU coordinates are computed wrt targets.

        Parameters
        ----------
              radar_grid : isce3.product.RadarGridParameters
                  Radar Grid
              geogrid : isce3.product.GeoGridParameters
                  Geogrid to be used as reference for output cubes
              heights : list
                  Cube heights
              orbit : isce3.core.Orbit
                  Orbit
              native_doppler : isce3.core.LUT2d
                  Native image Doppler
              grid_doppler : isce3.core.LUT2d
                  Grid Doppler
              epsg_los_and_along_track_vectors : int, optional
                  EPSG code for LOS and along-track unit vectors
                  (0 or 4326 for ENU coordinates)
              slant_range_raster : isce3.io.Raster, optional
                  Slant-range (in meters) cube raster
              azimuth_time_raster : isce3.io.Raster, optional
                  Azimuth time (in seconds) cube raster
              incidence_angle_raster : isce3.io.Raster, optional
                  Incidence angle (in degrees wrt ellipsoid normal at target)
                  cube raster
              los_unit_vector_x_raster : isce3.io.Raster, optional
                  LOS (target-to-sensor) unit vector X cube raster
              los_unit_vector_y_raster : isce3.io.Raster, optional
                  LOS (target-to-sensor) unit vector Y cube raster
              along_track_unit_vector_x_raster : isce3.io.Raster, optional
                  Along-track unit vector X raster
              along_track_unit_vector_y_raster : isce3.io.Raster, optional
                  Along-track unit vector Y raster
              elevation_angle_raster : isce3.io.Raster, optional
                  Elevation angle (in degrees wrt geodedic nadir) cube raster
              ground_track_velocity_raster : isce3.io.Raster, optional
                  Ground-track velocity cube raster
              threshold_geo2rdr : double, optional
                  Range threshold for geo2rdr
              numiter_geo2rdr : int, optional
                  Geo2rdr maximum number of iterations
              delta_range : double, optional
                  Step size used for computing derivative of doppler

)");

    m.def("make_geolocation_cubes", &isce3::geometry::makeGeolocationGridCubes,
            py::arg("radar_grid"), py::arg("heights"), py::arg("orbit"),
            py::arg("native_doppler"), py::arg("grid_doppler"), py::arg("epsg"),
            py::arg("epsg_los_and_along_track_vectors") = 0,
            py::arg("coordinate_x_raster") = nullptr,
            py::arg("coordinate_y_raster") = nullptr,
            py::arg("incidence_angle_raster") = nullptr,
            py::arg("lo_unit_vector_x_raster") = nullptr,
            py::arg("los_unit_vector_y_raster") = nullptr,
            py::arg("along_track_unit_vector_x_raster") = nullptr,
            py::arg("along_track_unit_vector_y_raster") = nullptr,
            py::arg("elevation_angle_raster") = nullptr,
            py::arg("ground_track_velocity_raster") = nullptr,
            py::arg("threshold_geo2rdr") = 1.0e-8,
            py::arg("numiter_geo2rdr") = 100, py::arg("delta_range") = 1.0e-6,
            R"(Make metadata geolocation grid cubes

           Metadata geolocation grid cubes describe the radar geometry
           over a three-dimensional grid, defined by
           a reference radar grid and a vector of heights.

           The representation as cubes, rather than two-dimensional rasters,
           is intended to reduce the amount of disk space required to
           store radar geometry values within NISAR L1 products.

           This is possible because each cube contains slow-varying values
           in space, that can be described by a low-resolution
           three-dimensional grid with sufficient accuracy.

           These values, however, are usually required at the
           terrain height, often characterized by a fast-varying surface
           representing the local topography. A high-resolution DEM can
           then be used to interpolate the metadata cubes and generate
           high-resolution maps of the corresponding radar geometry variable.

           The line-of-sight (LOS) and along-track unit vectors are referenced to
           the projection defined by the epsg_los_and_along_track_vectors code.
           In the case of ENU, i.e. epsg_los_and_along_track_vectors equals to 0
           or 4326, ENU coordinates are computed wrt targets.

        Parameters
        ----------
              radar_grid : isce3.product.RadarGridParameters
                  Cube radar grid
              heights : list
                  Cube heights
              orbit : isce3.core.Orbit
                  Orbit
              native_doppler : isce3.core.LUT2d
                  Native image Doppler
              grid_doppler : isce3.core.LUT2d
                  Grid Doppler
              epsg : int
                  Output geolocation EPSG
              epsg_los_and_along_track_vectors : int, optional
                  EPSG code for LOS and along-track unit vectors
                  (0 or 4326 for ENU coordinates)
              coordinate_x_raster : isce3.io.Raster, optional
                 Geolocation coordinate X raster
              coordinate_y_raster : isce3.io.Raster, optional
                  Geolocation coordinate Y raster
              incidence_angle_raster : isce3.io.Raster, optional
                  Incidence angle (in degrees wrt ellipsoid normal at target)
                  cube raster
              los_unit_vector_x_raster : isce3.io.Raster, optional
                  LOS (target-to-sensor) unit vector X cube raster
              los_unit_vector_y_raster : isce3.io.Raster, optional
                  LOS (target-to-sensor) unit vector Y cube raster
              along_track_unit_vector_x_raster : isce3.io.Raster, optional
                  Along-track unit vector X raster
              along_track_unit_vector_y_raster : isce3.io.Raster, optional
                  Along-track unit vector Y raster
              elevation_angle_raster : isce3.io.Raster, optional
                  Elevation angle (in degrees wrt geodedic nadir) cube raster
              ground_track_velocity_raster : isce3.io.Raster, optional
                  Ground-track velocity cube raster
              threshold_geo2rdr : double, optional
                  Range threshold for geo2rdr
              numiter_geo2rdr : int, optional
                  Geo2rdr maximum number of iterations
              delta_range : double, optional
                  Step size used for computing derivative of doppler

)");
}
