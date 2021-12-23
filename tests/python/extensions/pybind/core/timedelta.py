#!/usr/bin/env python3

import numpy.testing as npt
import pybind_isce3 as isce


def test_default_ctor():
    dt = isce.core.TimeDelta()
    print(dt)


def test_from_seconds():
    seconds = 1.23
    dt = isce.core.TimeDelta(seconds)

    assert dt.seconds == 1
    npt.assert_almost_equal(dt.frac, 0.23)


def test_from_hms():
    hours = 1
    minutes = 2
    seconds = 3
    dt = isce.core.TimeDelta(hours, minutes, seconds)

    assert dt.hours == 1
    assert dt.minutes == 2
    assert dt.seconds == 3


def test_from_hmsfrac():
    hours = 1
    minutes = 2
    seconds = 3
    frac = 0.4
    dt = isce.core.TimeDelta(hours, minutes, seconds, frac)

    assert dt.hours == 1
    assert dt.minutes == 2
    assert dt.seconds == 3
    npt.assert_almost_equal(dt.frac, 0.4)


def test_from_dhmsfrac():
    days = 1
    hours = 2
    minutes = 3
    seconds = 4
    frac = 0.5
    dt = isce.core.TimeDelta(days, hours, minutes, seconds, frac)

    assert dt.days == 1
    assert dt.hours == 2
    assert dt.minutes == 3
    assert dt.seconds == 4
    npt.assert_almost_equal(dt.frac, 0.5)


def test_from_datetime_timedelta():
    from datetime import timedelta

    cases = [
        timedelta(seconds=1.32),
        timedelta(minutes=36),  # breaks int32 microseconds
        timedelta(hours=1, minutes=15),  # breaks uint32 microseconds
        timedelta(days=365 * 70),  # breaks int32 seconds
        timedelta(days=365 * 140),  # breaks uint32 seconds
        timedelta(days=365 * 2e5),  # close to max int64 microseconds
        timedelta(days=1.23456),  # exercise all fields
    ]
    for pytd in cases:
        iscetd = isce.core.TimeDelta(pytd)
        # This should be okay for chosen cases (52 bit mantissa in double, all
        # but first case are integer seconds).
        npt.assert_almost_equal(
            iscetd.total_seconds(), pytd.total_seconds(), err_msg=f"case: {pytd}"
        )


def test_comparison():
    dt1 = isce.core.TimeDelta(1.0)
    dt2 = isce.core.TimeDelta(1.0)
    dt3 = isce.core.TimeDelta(2.0)

    assert dt3 > dt1
    assert dt2 < dt3
    assert dt1 >= dt2
    assert dt1 <= dt3
    assert dt1 == dt2
    assert dt1 != dt3


def test_add():
    dt1 = isce.core.TimeDelta(1.0)
    dt2 = isce.core.TimeDelta(2.0)
    dt3 = dt1 + dt2
    dt1 += dt2

    npt.assert_almost_equal(dt3.total_seconds(), 3.0)
    npt.assert_almost_equal(dt1.total_seconds(), 3.0)


def test_subtract():
    dt1 = isce.core.TimeDelta(1.0)
    dt2 = isce.core.TimeDelta(2.0)
    dt3 = dt1 - dt2
    dt1 -= dt2

    npt.assert_almost_equal(dt3.total_seconds(), -1.0)
    npt.assert_almost_equal(dt1.total_seconds(), -1.0)


def test_multiply():
    dt1 = isce.core.TimeDelta(1.0)
    dt2 = dt1 * 2.0
    dt1 *= 2.0

    npt.assert_almost_equal(dt2.total_seconds(), 2.0)
    npt.assert_almost_equal(dt1.total_seconds(), 2.0)


def test_divide():
    dt1 = isce.core.TimeDelta(1.0)
    dt2 = dt1 / 2.0
    dt1 /= 2.0

    npt.assert_almost_equal(dt2.total_seconds(), 0.5)
    npt.assert_almost_equal(dt1.total_seconds(), 0.5)


def test_total_days():
    dt = isce.core.TimeDelta(1, 6, 0, 0, 0.0)
    npt.assert_almost_equal(dt.total_days(), 1.25)


def test_total_hours():
    dt = isce.core.TimeDelta(1, 2, 30, 0, 0.0)
    npt.assert_almost_equal(dt.total_hours(), 26.5)


def test_total_minutes():
    dt = isce.core.TimeDelta(1, 2, 3, 30, 0.0)
    npt.assert_almost_equal(dt.total_minutes(), 1563.5)


def test_total_seconds():
    dt = isce.core.TimeDelta(1, 2, 3, 4, 0.5)
    npt.assert_almost_equal(dt.total_seconds(), 93784.5)
