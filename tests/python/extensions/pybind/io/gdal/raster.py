#!/usr/bin/env python3

from pathlib import Path

import iscetest
import numpy as np
import numpy.testing as npt
import pybind_isce3 as isce


def test_default_driver():
    driver = isce.io.gdal.Raster.default_driver()
    print(driver)


# init from path
# (access defaults to read-only)
def test_open1():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-dem"
    raster = isce.io.gdal.Raster(str(path))

    assert raster.access == isce.io.gdal.GDALAccess.GA_ReadOnly
    assert raster.datatype == isce.io.gdal.GDALDataType.GDT_Float32
    assert raster.width == 36
    assert raster.length == 72
    assert raster.driver == "ENVI"
    npt.assert_almost_equal(raster.x0, -156.000138888886)
    npt.assert_almost_equal(raster.y0, 20.0001388888836)
    npt.assert_almost_equal(raster.dx, 0.000277777777777815)
    npt.assert_almost_equal(raster.dy, -0.000277777777777815)


# init from path + access mode enum
def test_open2():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-dem"
    access = isce.io.gdal.GDALAccess.GA_ReadOnly
    raster = isce.io.gdal.Raster(str(path), access)

    assert raster.access == access


# init from path + access mode char
# (valid access modes are 'r', 'w')
def test_open3():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-dem"
    raster = isce.io.gdal.Raster(str(path), "r")

    assert raster.access == isce.io.gdal.GDALAccess.GA_ReadOnly

    # not mappable to GDALAccess
    with npt.assert_raises(RuntimeError):
        raster = isce.io.gdal.Raster(str(path), "z")


# init from path + band index
def test_open4():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-dem"
    band = 1
    raster = isce.io.gdal.Raster(str(path), band)

    assert raster.band == band

    # out-of-range band index
    band = 3
    with npt.assert_raises(IndexError):
        raster = isce.io.gdal.Raster(str(path), band)


# init from path + band index + access mode
def test_open5():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-dem"
    band = 1
    access = isce.io.gdal.GDALAccess.GA_Update
    raster = isce.io.gdal.Raster(str(path), band, access)

    assert raster.band == band
    assert raster.access == access


# init from path + band index + access mode char
def test_open5():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-dem"
    band = 1
    raster = isce.io.gdal.Raster(str(path), band, "r")

    assert raster.band == band
    assert raster.access == isce.io.gdal.GDALAccess.GA_ReadOnly


# create new raster using default driver
def test_create1():
    path = "Raster-create1"
    width = 4
    length = 8
    datatype = isce.io.gdal.GDALDataType.GDT_Float32
    raster = isce.io.gdal.Raster(path, width, length, datatype)

    assert raster.access == isce.io.gdal.GDALAccess.GA_Update
    assert raster.width == width
    assert raster.length == length
    assert raster.driver == isce.io.gdal.Raster.default_driver()


def test_create2():
    path = "Raster-create2"
    width = 4
    length = 8
    datatype = isce.io.gdal.GDALDataType.GDT_Float32
    driver = "GTiff"
    raster = isce.io.gdal.Raster(path, width, length, datatype, driver)

    assert raster.driver == driver


# datatype can be anything convertible to numpy.dtype
# (as long as it's mappable to GDALDataType)
def test_create3():
    path = "Raster-create3"
    width = 4
    length = 8

    raster = isce.io.gdal.Raster(path, width, length, float)
    raster = isce.io.gdal.Raster(path, width, length, "i4")
    raster = isce.io.gdal.Raster(path, width, length, np.complex64)

    # not mappable to GDALDataType
    with npt.assert_raises(RuntimeError):
        raster = isce.io.gdal.Raster(path, width, length, "int64")


def test_from_numpy():
    arr = np.empty((4, 5), dtype=float)
    raster = isce.io.gdal.Raster(arr)

    assert raster.datatype == isce.io.gdal.GDALDataType.GDT_Float64
    assert raster.access == isce.io.gdal.GDALAccess.GA_Update
    assert raster.width == arr.shape[1]
    assert raster.length == arr.shape[0]
    assert raster.driver == "MEM"
    npt.assert_almost_equal(raster.x0, 0.0)
    npt.assert_almost_equal(raster.y0, 0.0)
    npt.assert_almost_equal(raster.dx, 1.0)
    npt.assert_almost_equal(raster.dy, 1.0)


def test_to_numpy():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-sequence"
    raster = isce.io.gdal.Raster(str(path))
    arr = np.array(raster)

    assert arr.shape == (raster.length, raster.width)
    assert arr.dtype == np.int32
    npt.assert_array_equal(arr, raster.data)


def test_data():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-sequence"
    raster = isce.io.gdal.Raster(str(path))

    assert raster.data.shape == (raster.length, raster.width)
    assert raster.data.dtype == np.int32

    # read-only raster
    assert raster.data.flags["WRITEABLE"] == False


def test_dataset():
    path = Path(iscetest.data) / "io" / "gdal" / "ENVIRaster-dem"
    raster = isce.io.gdal.Raster(str(path))
    dataset = raster.dataset()

    assert dataset.access == raster.access
    assert dataset.width == raster.width
    assert dataset.length == raster.length
    assert dataset.driver == raster.driver
