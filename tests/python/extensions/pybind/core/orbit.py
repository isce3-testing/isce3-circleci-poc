#!/usr/bin/env python3

import numpy.testing as npt


def load_h5():
    from os import path

    import h5py
    from iscetest import data
    from pybind_isce3.core import Orbit

    f = h5py.File(path.join(data, "envisat.h5"), "r")
    return Orbit.load_from_h5(f["/science/LSAR/SLC/metadata/orbit"])


o = load_h5()


def test_save():
    import tempfile

    import h5py

    o = load_h5()
    _, name = tempfile.mkstemp()
    with h5py.File(name, "w") as h5:
        g = h5.create_group("/orbit")
        o.save_to_h5(g)


# Test that accessors exist
def test_props():
    o = load_h5()
    o.time
    o.position
    o.velocity


# Test that loaded data is valid
def test_members():
    import numpy
    from numpy.linalg import norm

    o = load_h5()

    # Check valid earth orbit distance
    earth_radius = 6_000e3  # meters
    geostationary = 35_000e3  # meters
    altitude = numpy.array([norm(pos) for pos in o.position])
    assert all(altitude > earth_radius)
    assert all(altitude < geostationary)

    # Check valid orbital velocity
    # How fast should a satellite move?
    # Probably faster than a car, but slower than the speed of light.
    car = 100 * 1000.0 / 3600.0  # km/h to m/s
    light = 3e8
    velocity = numpy.array([norm(vel) for vel in o.velocity])
    assert all(velocity > car)
    assert all(velocity < light)
