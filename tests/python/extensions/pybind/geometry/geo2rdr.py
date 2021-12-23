#!/usr/bin/env python3

import os

import iscetest
import numpy as np
import numpy.testing as npt
import pybind_isce3 as isce3
from nisar.products.readers import SLC
from osgeo import gdal


def test_point():
    h5_path = os.path.join(iscetest.data, "envisat.h5")

    radargrid = isce3.product.RadarGridParameters(h5_path)

    slc = SLC(hdf5file=h5_path)
    orbit = slc.getOrbit()
    doppler = slc.getDopplerCentroid()

    ellipsoid = isce3.core.Ellipsoid()

    llh = np.array(
        [np.deg2rad(-115.72466801139711), np.deg2rad(34.65846532785868), 1772.0]
    )

    # run with native doppler
    aztime, slant_range = isce3.geometry.geo2rdr(
        llh,
        ellipsoid,
        orbit,
        doppler,
        radargrid.wavelength,
        radargrid.lookside,
        threshold=1.0e-10,
        maxiter=50,
        delta_range=10.0,
    )

    azdate = orbit.reference_epoch + isce3.core.TimeDelta(aztime)
    assert azdate.isoformat() == "2003-02-26T17:55:33.993088889"
    npt.assert_almost_equal(slant_range, 830450.1859446081, decimal=6)

    # run again with zero doppler
    doppler = isce3.core.LUT2d()

    aztime, slant_range = isce3.geometry.geo2rdr(
        llh,
        ellipsoid,
        orbit,
        doppler,
        radargrid.wavelength,
        radargrid.lookside,
        threshold=1.0e-10,
        maxiter=50,
        delta_range=10.0,
    )

    azdate = orbit.reference_epoch + isce3.core.TimeDelta(aztime)
    assert azdate.isoformat() == "2003-02-26T17:55:34.122893704"
    npt.assert_almost_equal(slant_range, 830449.6727720434, decimal=6)


def test_run():
    """
    check if geo2rdr runs
    """
    # prepare Geo2Rdr init params
    h5_path = os.path.join(iscetest.data, "envisat.h5")

    radargrid = isce3.product.RadarGridParameters(h5_path)

    slc = SLC(hdf5file=h5_path)
    orbit = slc.getOrbit()
    doppler = slc.getDopplerCentroid()

    ellipsoid = isce3.core.Ellipsoid()

    # init Geo2Rdr class
    geo2rdr_obj = isce3.geometry.Geo2Rdr(
        radargrid, orbit, ellipsoid, doppler, threshold=1e-9, numiter=50
    )

    # load rdr2geo unit test output
    rdr2geo_raster = isce3.io.Raster("topo.vrt")

    # run
    geo2rdr_obj.geo2rdr(rdr2geo_raster, ".")


def test_validate():
    """
    validate generated results
    """
    # list of test outputs
    test_outputs = ["range.off", "azimuth.off"]

    # check each generated raster
    for test_output in test_outputs:
        # load dataset and get array
        test_ds = gdal.Open(test_output, gdal.GA_ReadOnly)
        test_arr = test_ds.GetRasterBand(1).ReadAsArray()

        # mask bad values
        test_arr = np.ma.masked_array(test_arr, mask=np.abs(test_arr) > 999.0)

        # accumulate error
        test_err = np.sum(test_arr * test_arr)

        assert test_err < 1e-9, f"{test_output} accumulated error fail"


if __name__ == "__main__":
    test_run()
    test_validate()
