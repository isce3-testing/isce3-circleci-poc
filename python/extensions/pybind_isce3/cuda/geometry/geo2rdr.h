#pragma once

#include <pybind11/pybind11.h>

#include <isce3/cuda/geometry/Geo2rdr.h>

void addbinding(pybind11::class_<isce3::cuda::geometry::Geo2rdr>&);
