#pragma once

#include <pybind11/pybind11.h>

#include <isce3/core/Ellipsoid.h>

void addbinding(pybind11::class_<isce3::core::Ellipsoid>&);
