#pragma once

#include <pybind11/pybind11.h>

#include <isce3/core/Attitude.h>

void addbinding(pybind11::class_<isce3::core::Attitude>&);
