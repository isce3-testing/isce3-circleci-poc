#pragma once

#include <pybind11/pybind11.h>

#include <isce3/io/gdal/Raster.h>

namespace py = pybind11;

void addbinding(py::class_<isce3::io::gdal::Raster>&);

py::buffer_info toBuffer(isce3::io::gdal::Raster&);

isce3::io::gdal::Raster toRaster(py::buffer);
