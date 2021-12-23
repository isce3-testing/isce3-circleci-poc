#!/usr/bin/env python3

import os
import time

import h5py
import isce3
import journal
from nisar.products.readers import SLC
from nisar.workflows import h5_prep
from nisar.workflows.gslc_runconfig import GSLCRunConfig
from nisar.workflows.h5_prep import add_radar_grid_cubes_to_hdf5
from nisar.workflows.yaml_argparse import YamlArgparse


def run(cfg):
    """
    run geocodeSlc according to parameters in cfg dict
    """
    # pull parameters from cfg
    input_hdf5 = cfg["InputFileGroup"]["InputFilePath"]
    output_hdf5 = cfg["ProductPathGroup"]["SASOutputFile"]
    freq_pols = cfg["processing"]["input_subset"]["list_of_frequencies"]
    geogrids = cfg["processing"]["geocode"]["geogrids"]
    radar_grid_cubes_geogrid = cfg["processing"]["radar_grid_cubes"]["geogrid"]
    radar_grid_cubes_heights = cfg["processing"]["radar_grid_cubes"]["heights"]
    dem_file = cfg["DynamicAncillaryFileGroup"]["DEMFile"]
    threshold_geo2rdr = cfg["processing"]["geo2rdr"]["threshold"]
    iteration_geo2rdr = cfg["processing"]["geo2rdr"]["maxiter"]
    lines_per_block = cfg["processing"]["blocksize"]["y"]
    dem_block_margin = cfg["processing"]["dem_margin"]
    flatten = cfg["processing"]["flatten"]

    # init parameters shared by frequency A and B
    slc = SLC(hdf5file=input_hdf5)
    orbit = slc.getOrbit()
    dem_raster = isce3.io.Raster(dem_file)
    epsg = dem_raster.get_epsg()
    proj = isce3.core.make_projection(epsg)
    ellipsoid = proj.ellipsoid

    # Doppler of the image grid (Zero for NISAR)
    image_grid_doppler = isce3.core.LUT2d()

    info_channel = journal.info("gslc.run")
    info_channel.log("starting geocode SLC")

    t_all = time.time()
    with h5py.File(output_hdf5, "a") as dst_h5:
        for freq in freq_pols.keys():
            frequency = f"frequency{freq}"
            pol_list = freq_pols[freq]
            radar_grid = slc.getRadarGrid(freq)
            geo_grid = geogrids[freq]

            # get doppler centroid
            native_doppler = slc.getDopplerCentroid(frequency=freq)

            for polarization in pol_list:
                t_pol = time.time()

                output_dir = os.path.dirname(os.path.abspath(output_hdf5))
                os.makedirs(output_dir, exist_ok=True)

                raster_ref = f"HDF5:{input_hdf5}:/{slc.slcPath(freq, polarization)}"
                slc_raster = isce3.io.Raster(raster_ref)

                # access the HDF5 dataset for a given frequency and polarization
                dataset_path = f"/science/LSAR/GSLC/grids/{frequency}/{polarization}"
                gslc_dataset = dst_h5[dataset_path]

                # Construct the output ratster directly from HDF5 dataset
                gslc_raster = isce3.io.Raster(
                    f"IH5:::ID={gslc_dataset.id.id}".encode("utf-8"), update=True
                )

                # run geocodeSlc
                isce3.geocode.geocode_slc(
                    gslc_raster,
                    slc_raster,
                    dem_raster,
                    radar_grid,
                    geo_grid,
                    orbit,
                    native_doppler,
                    image_grid_doppler,
                    ellipsoid,
                    threshold_geo2rdr,
                    iteration_geo2rdr,
                    lines_per_block,
                    dem_block_margin,
                    flatten,
                )

                # the rasters need to be deleted
                del gslc_raster
                del slc_raster

                # output_raster_ref = f'HDF5:{output_hdf5}:/{dataset_path}'
                gslc_raster = isce3.io.Raster(
                    f"IH5:::ID={gslc_dataset.id.id}".encode("utf-8")
                )
                _compute_stats(gslc_raster, gslc_dataset)

                t_pol_elapsed = time.time() - t_pol
                info_channel.log(
                    f"polarization {polarization} ran in {t_pol_elapsed:.3f} seconds"
                )

            if freq.upper() == "B":
                continue

            cube_geogrid = isce3.product.GeoGridParameters(
                start_x=radar_grid_cubes_geogrid.start_x,
                start_y=radar_grid_cubes_geogrid.start_y,
                spacing_x=radar_grid_cubes_geogrid.spacing_x,
                spacing_y=radar_grid_cubes_geogrid.spacing_y,
                width=int(radar_grid_cubes_geogrid.width),
                length=int(radar_grid_cubes_geogrid.length),
                epsg=radar_grid_cubes_geogrid.epsg,
            )

            cube_group_name = "/science/LSAR/GSLC/metadata/radarGrid"

            native_doppler.bounds_error = False
            """
            The native-Doppler LUT bounds error is turned off to
            computer cubes values outside radar-grid boundaries
            """
            add_radar_grid_cubes_to_hdf5(
                dst_h5,
                cube_group_name,
                cube_geogrid,
                radar_grid_cubes_heights,
                radar_grid,
                orbit,
                native_doppler,
                image_grid_doppler,
                threshold_geo2rdr,
                iteration_geo2rdr,
            )

    t_all_elapsed = time.time() - t_all
    info_channel.log(f"successfully ran geocode SLC in {t_all_elapsed:.3f} seconds")


def _compute_stats(raster, h5_ds):
    stats_obj = isce3.math.compute_raster_stats_real_imag(raster)[0]
    h5_ds.attrs.create("min_real_value", data=stats_obj.min_real)
    h5_ds.attrs.create("mean_real_value", data=stats_obj.mean_real)
    h5_ds.attrs.create("max_real_value", data=stats_obj.max_real)
    h5_ds.attrs.create("sample_stddev_real", data=stats_obj.sample_stddev_real)
    h5_ds.attrs.create("min_imag_value", data=stats_obj.min_imag)
    h5_ds.attrs.create("mean_imag_value", data=stats_obj.mean_imag)
    h5_ds.attrs.create("max_imag_value", data=stats_obj.max_imag)
    h5_ds.attrs.create("sample_stddev_imag", data=stats_obj.sample_stddev_imag)


if __name__ == "__main__":
    yaml_parser = YamlArgparse()
    args = yaml_parser.parse()
    gslc_runcfg = GSLCRunConfig(args)
    h5_prep.run(gslc_runcfg.cfg)
    run(gslc_runcfg.cfg)
