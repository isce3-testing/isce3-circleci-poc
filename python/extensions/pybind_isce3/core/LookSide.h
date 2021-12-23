#pragma once

#include <pybind11/pybind11.h>

#include <isce3/core/LookSide.h>

void addbinding(pybind11::enum_<isce3::core::LookSide>&);

isce3::core::LookSide duck_look_side(pybind11::object);
