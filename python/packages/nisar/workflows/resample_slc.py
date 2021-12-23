#!/usr/bin/env python3

"""
wrapper for resample
"""

import os
import pathlib
import time

import isce3
import journal
from nisar.products.readers import SLC
from nisar.workflows import gpu_check, runconfig
from nisar.workflows.resample_slc_runconfig import ResampleSlcRunConfig
from nisar.workflows.yaml_argparse import YamlArgparse
from osgeo import gdal


def run(cfg, resample_type):
    """
    run resample_slc
    """
    input_hdf5 = cfg["InputFileGroup"]["SecondaryFilePath"]
    scratch_path = pathlib.Path(cfg["ProductPathGroup"]["ScratchPath"])
    freq_pols = cfg["processing"]["input_subset"]["list_of_frequencies"]

    # According to the type of resampling, choose proper resample cfg
    resamp_args = cfg["processing"][f"{resample_type}_resample"]

    # Get SLC parameters
    slc = SLC(hdf5file=input_hdf5)

    info_channel = journal.info("resample_slc.run")
    info_channel.log("starting resampling SLC")

    # Check if use GPU or CPU resampling
    use_gpu = gpu_check.use_gpu(cfg["worker"]["gpu_enabled"], cfg["worker"]["gpu_id"])

    if use_gpu:
        # Set current CUDA device
        device = isce3.cuda.core.Device(cfg["worker"]["gpu_id"])
        isce3.cuda.core.set_device(device)

    t_all = time.time()

    for freq in freq_pols.keys():
        # Get frequency specific parameters
        radar_grid = slc.getRadarGrid(frequency=freq)
        native_doppler = slc.getDopplerCentroid(frequency=freq)

        # Open offsets
        offsets_dir = pathlib.Path(resamp_args["offsets_dir"])

        # Create separate directories for coarse and fine resample
        # Open corresponding range/azimuth offsets
        resample_slc_scratch_path = (
            scratch_path / f"{resample_type}_resample_slc" / f"freq{freq}"
        )
        if resample_type == "coarse":
            offsets_path = offsets_dir / "geo2rdr" / f"freq{freq}"
            rg_off = isce3.io.Raster(str(offsets_path / "range.off"))
            az_off = isce3.io.Raster(str(offsets_path / "azimuth.off"))
        else:
            # We checked the existence of HH/VV offsets in resample_slc_runconfig.py
            # Select the first offsets available between HH and VV
            freq_offsets_path = offsets_dir / "rubbersheet_offsets" / f"freq{freq}"
            if os.path.isdir(str(freq_offsets_path / "HH")):
                offsets_path = freq_offsets_path / "HH"
            else:
                offsets_path = freq_offsets_path / "VV"
            rg_off = isce3.io.Raster(str(offsets_path / "range.off.vrt"))
            az_off = isce3.io.Raster(str(offsets_path / "azimuth.off.vrt"))

        # Create resample slc directory
        resample_slc_scratch_path.mkdir(parents=True, exist_ok=True)

        # Initialize CPU or GPU resample object accordingly
        if use_gpu:
            Resamp = isce3.cuda.image.ResampSlc
        else:
            Resamp = isce3.image.ResampSlc

        resamp_obj = Resamp(radar_grid, native_doppler)

        # If lines per tile is > 0, assign it to resamp_obj
        if resamp_args["lines_per_tile"]:
            resamp_obj.lines_per_tile = resamp_args["lines_per_tile"]

        # Get polarization list for which resample SLCs
        pol_list = freq_pols[freq]

        for pol in pol_list:
            # Create directory for each polarization
            out_dir = resample_slc_scratch_path / pol
            out_dir.mkdir(parents=True, exist_ok=True)
            out_path = out_dir / "coregistered_secondary.slc"

            # Extract and create raster of SLC to resample
            h5_ds = f"/{slc.SwathPath}/frequency{freq}/{pol}"
            raster_path = f"HDF5:{input_hdf5}:{h5_ds}"
            raster = isce3.io.Raster(raster_path)

            # Create output raster
            resamp_slc = isce3.io.Raster(
                str(out_path),
                rg_off.width,
                rg_off.length,
                rg_off.num_bands,
                gdal.GDT_CFloat32,
                "ENVI",
            )
            resamp_obj.resamp(raster, resamp_slc, rg_off, az_off)

    t_all_elapsed = time.time() - t_all
    info_channel.log(f"successfully ran resample in {t_all_elapsed:.3f} seconds")


if __name__ == "__main__":
    """
    run resample_slc from command line
    """

    # load command line args
    resample_slc_parser = YamlArgparse(resample_type=True)
    args = resample_slc_parser.parse()

    # Extract resample_type
    resample_type = args.resample_type

    # Get a runconfig dictionary from command line args
    resample_slc_runconfig = ResampleSlcRunConfig(args, resample_type)

    # Run resample_slc
    run(resample_slc_runconfig.cfg, resample_type)
