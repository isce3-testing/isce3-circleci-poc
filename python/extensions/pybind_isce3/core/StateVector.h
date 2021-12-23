#pragma once

#include <pybind11/pybind11.h>

#include <isce3/core/StateVector.h>

void addbinding(pybind11::class_<isce3::core::StateVector>&);
