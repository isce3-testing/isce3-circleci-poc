#!/usr/bin/env python3

import iscetest
import numpy.testing as npt
import pybind_isce3 as isce


def test_radargridparameters():
    # Create RadarGridParameters object
    swath = isce.product.Swath(iscetest.data + "envisat.h5", "A")

    # Check its values
    npt.assert_equal(swath.slant_range[0], 826988.6900674499)
    npt.assert_equal(swath.zero_doppler_time[0], 237330.843491759)
    npt.assert_equal(swath.acquired_center_frequency, 5.331004416e9)
    npt.assert_equal(swath.processed_center_frequency, 5.331004416e9)
    npt.assert_almost_equal(swath.acquired_range_bandwidth, 1.6e7, 1)
    npt.assert_almost_equal(swath.processed_range_bandwidth, 1.6e7, 1)
    npt.assert_equal(swath.nominal_acquisition_prf, 1.0 / 6.051745968279355e-4)
    npt.assert_equal(swath.scene_center_ground_range_spacing, 23.774273647897644)


# end of file
