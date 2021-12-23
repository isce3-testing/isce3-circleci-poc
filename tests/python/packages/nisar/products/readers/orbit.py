from pathlib import Path

import isce3
import iscetest
import nisar
import numpy.testing as npt


def test_load_orbit():
    fn = Path(iscetest.data) / "orbit.xml"
    orbit = nisar.products.readers.load_orbit_from_xml(fn)

    # Some spot checks based on visual inspection of the XML file.
    t0 = isce3.core.DateTime(2015, 12, 10, 20, 59, 43.0)
    assert orbit.reference_epoch == t0
    npt.assert_allclose(
        orbit.position[0], [6050436.50829497, -49963.56327277, -4790189.19514634]
    )
    npt.assert_allclose(
        orbit.velocity[0], [-3789.26877184, 3311.88961993, -4818.57127302]
    )
    # By default first time stamp should be zero.
    assert orbit.time[0] == 0.0

    # Now try reading with a specified epoch.
    epoch = t0 + isce3.core.TimeDelta(seconds=1.0)
    orbit = nisar.products.readers.load_orbit_from_xml(fn, epoch)
    assert orbit.reference_epoch == epoch
    assert orbit.time[0] == -1.0
