#pragma once

#include <isce3/core/forward.h>
#include <isce3/io/forward.h>
#include <isce3/product/forward.h>

#include <vector>

namespace isce3 { namespace geometry {

/**  Compute geometry vectors and metadata cube values for a given target
 * point and write to the corresponding arrays.
 *
 * @param[in]  array_pos_i                      Output array line
 * @param[in]  array_pos_j                      Output array column
 * @param[in]  native_azimuth_time              Native Doppler azimuth time
 * @param[in]  target_llh                       Target lat/lon/height vector
 * @param[in]  target_proj                      Target coordinates in the output
 * projection system
 * @param[in]  orbit                            Reference orbit
 * @param[in]  ellipsoid                        Reference ellipsoid
 * @param[in]  proj_los_and_along_track_vectors ProjectionBase object for
 * representing LOS and along-track unit vectors
 * @param[out] incidence_angle_raster           Incidence angle cube raster
 * @param[out] incidence_angle_array            Incidence angle cube array
 * @param[out] los_unit_vector_x_raster         LOS (target-to-sensor) unit
 * vector X cube raster
 * @param[out] los_unit_vector_x_array          LOS (target-to-sensor) unit
 * vector X cube array
 * @param[out] los_unit_vector_y_raster         LOS (target-to-sensor) unit
 * vector Y cube raster
 * @param[out] los_unit_vector_y_array          LOS (target-to-sensor) unit
 * vector Y cube array
 * @param[out] along_track_unit_vector_x_raster Along-track unit vector X raster
 * @param[out] along_track_unit_vector_x_array  Along-track unit vector X array
 * @param[out] along_track_unit_vector_y_raster Along-track unit vector Y raster
 * @param[out] along_track_unit_vector_y_array  Along-track unit vector Y array
 * @param[out] elevation_angle_raster           Elevation cube raster
 * @param[out] elevation_angle_array            Elevation cube array
 * @param[out] ground_track_velocity_raster     Ground-track velocity raster
 * @param[out] ground_track_velocity_array      Ground-track velocity array
 */
inline void writeVectorDerivedCubes(const int array_pos_i,
        const int array_pos_j, const double native_azimuth_time,
        const isce3::core::Vec3& target_llh,
        const isce3::core::Vec3& target_proj, const isce3::core::Orbit& orbit,
        const isce3::core::Ellipsoid& ellipsoid,
        const isce3::core::ProjectionBase* proj_los_and_along_track_vectors,
        isce3::io::Raster* incidence_angle_raster,
        isce3::core::Matrix<float>& incidence_angle_array,
        isce3::io::Raster* los_unit_vector_x_raster,
        isce3::core::Matrix<float>& los_unit_vector_x_array,
        isce3::io::Raster* los_unit_vector_y_raster,
        isce3::core::Matrix<float>& los_unit_vector_y_array,
        isce3::io::Raster* along_track_unit_vector_x_raster,
        isce3::core::Matrix<float>& along_track_unit_vector_x_array,
        isce3::io::Raster* along_track_unit_vector_y_raster,
        isce3::core::Matrix<float>& along_track_unit_vector_y_array,
        isce3::io::Raster* elevation_angle_raster,
        isce3::core::Matrix<float>& elevation_angle_array,
        isce3::io::Raster* ground_track_velocity_raster,
        isce3::core::Matrix<double>& ground_track_velocity_array);

/** Make metadata radar grid cubes
 *
 * Metadata radar grid cubes describe the radar geometry
 * over a three-dimensional grid, defined by
 * a reference geogrid and a vector of heights.
 *
 * The representation as cubes, rather than two-dimensional rasters,
 * is intended to reduce the amount of disk space required to
 * store radar geometry values within NISAR L2 products.
 *
 * This is possible because each cube contains slow-varying values
 * in space, that can be described by a low-resolution
 * three-dimensional grid with sufficient accuracy.
 *
 * These values, however, are usually required at the
 * terrain height, often characterized by a fast-varying surface
 * representing the local topography. A high-resolution DEM can
 * then be used to interpolate the metadata cubes and generate
 * high-resolution maps of the corresponding radar geometry variable.
 *
 * The line-of-sight (LOS) and along-track unit vectors are referenced to
 * the projection defined by the epsg_los_and_along_track_vectors code.
 * In the case of ENU, i.e. epsg_los_and_along_track_vectors equals to 0
 * or 4326, ENU coordinates are computed wrt targets.
 *
 * @param[in]  radar_grid                  Radar grid
 * @param[in]  geogrid                     Geogrid to be
 *  used as reference for output cubes
 * @param[in]  heights                     Cube heights [m]
 * @param[in]  orbit                       Reference orbit
 * @param[in]  native_doppler              Native image Doppler
 * @param[in]  grid_doppler                Grid Doppler
 * @param[in]  epsg_los_and_along_track_vectors EPSG code for LOS and
 * along-track unit vectors (0 or 4326 for ENU coordinates)
 * @param[out] slant_range_raster          Slant-range (in meters)
 * cube raster
 * @param[out] azimuth_time_raster         Azimuth time (in seconds)
 * cube raster
 * @param[out] incidence_angle_raster      Incidence angle (in degrees wrt
 * ellipsoid normal at target) cube raster
 * @param[out] los_unit_vector_x_raster    LOS (target-to-sensor) unit vector
 * X cube raster
 * @param[out] los_unit_vector_y_raster    LOS (target-to-sensor) unit vector
 * Y cube raster
 * @param[out] along_track_unit_vector_x_raster Along-track unit vector X
 * cube raster
 * @param[out] along_track_unit_vector_y_raster Along-track unit vector Y
 * cube raster
 * @param[out] elevation_angle_raster      Elevation angle (in degrees wrt
 * geodedic nadir) cube raster
 * @param[out] ground_track_velocity_raster Ground-track velocity raster
 * @param[in]  threshold_geo2rdr           Range threshold for geo2rdr
 * @param[in]  numiter_geo2rdr             Geo2rdr maximum number of iterations
 * @param[in]  delta_range                 Step size used for computing
 * derivative of doppler
 */
void makeRadarGridCubes(const isce3::product::RadarGridParameters& radar_grid,
        const isce3::product::GeoGridParameters& geogrid,
        const std::vector<double>& heights, const isce3::core::Orbit& orbit,
        const isce3::core::LUT2d<double>& native_doppler,
        const isce3::core::LUT2d<double>& grid_doppler,
        const int epsg_los_and_along_track_vectors = 0,
        isce3::io::Raster* slant_range_raster = nullptr,
        isce3::io::Raster* azimuth_time_raster = nullptr,
        isce3::io::Raster* incidence_angle_raster = nullptr,
        isce3::io::Raster* los_unit_vector_x_raster = nullptr,
        isce3::io::Raster* los_unit_vector_y_raster = nullptr,
        isce3::io::Raster* along_track_unit_vector_x_raster = nullptr,
        isce3::io::Raster* along_track_unit_vector_y_raster = nullptr,
        isce3::io::Raster* elevation_angle_raster = nullptr,
        isce3::io::Raster* ground_track_velocity_raster = nullptr,
        const double threshold_geo2rdr = 1e-8, const int numiter_geo2rdr = 100,
        const double delta_range = 1e-8);

/** Make metadata geolocation grid cubes
 *
 * Metadata geolocation grid cubes describe the radar geometry
 * over a three-dimensional grid, defined by
 * a reference radar grid and a vector of heights.
 *
 * The representation as cubes, rather than two-dimensional rasters,
 * is intended to reduce the amount of disk space required to
 * store radar geometry values within NISAR L1 products.
 *
 * This is possible because each cube contains slow-varying values
 * in space, that can be described by a low-resolution
 * three-dimensional grid with sufficient accuracy.
 *
 * These values, however, are usually required at the
 * terrain height, often characterized by a fast-varying surface
 * representing the local topography. A high-resolution DEM can
 * then be used to interpolate the metadata cubes and generate
 * high-resolution maps of the corresponding radar geometry variable.
 *
 * The line-of-sight (LOS) and along-track unit vectors are referenced to
 * the projection defined by the epsg_los_and_along_track_vectors code.
 * In the case of ENU, i.e. epsg_los_and_along_track_vectors equals to 0
 * or 4326, ENU coordinates are computed wrt targets.
 *
 * @param[in]  radar_grid                Cube radar grid
 * @param[in]  heights                   Cube heights
 * @param[in]  orbit                     Orbit
 * @param[in]  native_doppler            Native image Doppler
 * @param[in]  grid_doppler              Grid Doppler.
 * @param[in]  epsg                      Output geolocation EPSG
 * @param[in]  epsg_los_and_along_track_vectors EPSG code for LOS and
 * along-track unit vectors (0 or 4326 for ENU coordinates)
 * @param[out] coordinate_x_raster       Geolocation coordinate X raster
 * @param[out] coordinate_y_raster       Geolocation coordinage Y raster
 * @param[out] incidence_angle_raster    Incidence angle (in degrees wrt
 * ellipsoid normal at target) cube raster
 * @param[out] los_unit_vector_x_raster    LOS (target-to-sensor) unit vector
 * X cube raster
 * @param[out] los_unit_vector_y_raster    LOS (target-to-sensor) unit vector
 * Y cube raster
 * @param[out] along_track_unit_vector_x_raster Along-track unit vector X
 * cube raster
 * @param[out] along_track_unit_vector_y_raster Along-track unit vector Y
 * cube raster
 * @param[out] elevation_angle_raster    Elevation angle (in degrees wrt
 * geodedic nadir) cube raster
 * @param[out] ground_track_velocity_raster Ground-track velocity raster
 * @param[in]  threshold_geo2rdr         Range threshold for geo2rdr
 * @param[in]  numiter_geo2rdr           Geo2rdr maximum number of iterations
 * @param[in]  delta_range               Step size used for computing
 * derivative of doppler
 */
void makeGeolocationGridCubes(
        const isce3::product::RadarGridParameters& radar_grid,
        const std::vector<double>& heights, const isce3::core::Orbit& orbit,
        const isce3::core::LUT2d<double>& native_doppler,
        const isce3::core::LUT2d<double>& grid_doppler, const int epsg,
        const int epsg_los_and_along_track_vectors = 0,
        isce3::io::Raster* coordinate_x_raster = nullptr,
        isce3::io::Raster* coordinate_y_raster = nullptr,
        isce3::io::Raster* incidence_angle_raster = nullptr,
        isce3::io::Raster* los_unit_vector_x_raster = nullptr,
        isce3::io::Raster* los_unit_vector_y_raster = nullptr,
        isce3::io::Raster* along_track_unit_vector_x_raster = nullptr,
        isce3::io::Raster* along_track_unit_vector_y_raster = nullptr,
        isce3::io::Raster* elevation_angle_raster = nullptr,
        isce3::io::Raster* ground_track_velocity_raster = nullptr,
        const double threshold_geo2rdr = 1e-8, const int numiter_geo2rdr = 100,
        const double delta_range = 1e-8);

}} // namespace isce3::geometry
