#!/usr/bin/env python3
import argparse
import os

import iscetest
from nisar.workflows import defaults, gslc, h5_prep
from nisar.workflows.gslc_runconfig import GSLCRunConfig


def test_run():
    """
    run gslc with same rasters and DEM as geocodeSlc test
    """
    test_yaml = os.path.join(iscetest.data, "geocodeslc/test_gslc.yaml")

    # load text then substitude test directory paths since data dir is read only
    with open(test_yaml) as fh_test_yaml:
        test_yaml = "".join(
            [line.replace("@ISCETEST@", iscetest.data) for line in fh_test_yaml]
        )

    # create CLI input namespace with yaml text instead of file path
    args = argparse.Namespace(run_config_path=test_yaml, log_file=False)

    # init runconfig object
    runconfig = GSLCRunConfig(args)
    runconfig.geocode_common_arg_load()

    # geocode same 2 rasters as C++/pybind geocodeSlc
    for xy in ["x", "y"]:
        # adjust runconfig to match just created raster
        runconfig.cfg["ProductPathGroup"]["SASOutputFile"] = f"{xy}_out.h5"

        # prepare output hdf5
        h5_prep.run(runconfig.cfg)

        # geocode test raster
        gslc.run(runconfig.cfg)


if __name__ == "__main__":
    test_run()
