#pragma once

#include <pybind11/pybind11.h>

#include <isce3/core/DateTime.h>

void addbinding(pybind11::class_<isce3::core::DateTime>&);
