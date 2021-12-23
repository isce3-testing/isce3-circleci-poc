#!/usr/bin/env python3

"""
Wrapper for geo2rdr
"""

import pathlib
import time

import isce3
import journal
from nisar.products.readers import SLC
from nisar.workflows import gpu_check, runconfig
from nisar.workflows.geo2rdr_runconfig import Geo2rdrRunConfig
from nisar.workflows.yaml_argparse import YamlArgparse


def run(cfg):
    """
    run geo2rdr
    """

    # Pull parameters from cfg dict
    sec_hdf5 = cfg["InputFileGroup"]["SecondaryFilePath"]
    dem_file = cfg["DynamicAncillaryFileGroup"]["DEMFile"]
    scratch_path = pathlib.Path(cfg["ProductPathGroup"]["ScratchPath"])
    freq_pols = cfg["processing"]["input_subset"]["list_of_frequencies"]
    threshold = cfg["processing"]["geo2rdr"]["threshold"]
    numiter = cfg["processing"]["geo2rdr"]["maxiter"]
    lines_per_block = cfg["processing"]["geo2rdr"]["lines_per_block"]

    # Get parameters from SLC
    slc = SLC(hdf5file=sec_hdf5)
    orbit = slc.getOrbit()

    # Set ellipsoid based on DEM epsg
    dem_raster = isce3.io.Raster(dem_file)
    epsg = dem_raster.get_epsg()
    proj = isce3.core.make_projection(epsg)
    ellipsoid = proj.ellipsoid

    # NISAR RSLC products are always zero doppler
    doppler_grid = isce3.core.LUT2d()

    info_channel = journal.info("geo2rdr.run")
    info_channel.log("starting geo2rdr")

    # check if gpu use if required
    use_gpu = gpu_check.use_gpu(cfg["worker"]["gpu_enabled"], cfg["worker"]["gpu_id"])

    if use_gpu:
        # set CUDA device
        device = isce3.cuda.core.Device(cfg["worker"]["gpu_id"])
        isce3.cuda.core.set_device(device)

    t_all = time.time()

    for freq in freq_pols.keys():

        # Get parameters specific for that frequency
        radar_grid = slc.getRadarGrid(frequency=freq)

        # Create geo2rdr directory
        geo2rdr_scratch_path = scratch_path / "geo2rdr" / f"freq{freq}"
        geo2rdr_scratch_path.mkdir(parents=True, exist_ok=True)

        # Initialize CPU or GPU geo2rdr object accordingly
        if use_gpu:
            Geo2Rdr = isce3.cuda.geometry.Geo2Rdr
        else:
            Geo2Rdr = isce3.geometry.Geo2Rdr

        geo2rdr_obj = Geo2Rdr(
            radar_grid,
            orbit,
            ellipsoid,
            doppler_grid,
            threshold,
            numiter,
            lines_per_block,
        )

        # Opem Topo Raster
        topo_path = pathlib.Path(cfg["processing"]["geo2rdr"]["topo_path"])
        rdr2geo_topo_path = topo_path / "rdr2geo" / f"freq{freq}" / "topo.vrt"
        topo_raster = isce3.io.Raster(str(rdr2geo_topo_path))

        # Run geo2rdr
        geo2rdr_obj.geo2rdr(topo_raster, str(geo2rdr_scratch_path))

    t_all_elapsed = time.time() - t_all
    info_channel.log(f"Successfully ran geo2rdr in {t_all_elapsed:.3f} seconds")


if __name__ == "__main__":
    """
    run geo2rdr from command line
    """

    # load command line args
    geo2rdr_parser = YamlArgparse()
    args = geo2rdr_parser.parse()

    # Get a runconfig dictionary from command line args
    geo2rdr_runconfig = Geo2rdrRunConfig(args)

    # Run geo2rdr
    run(geo2rdr_runconfig.cfg)
