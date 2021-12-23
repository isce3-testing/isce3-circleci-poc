#pragma once

#include <pybind11/pybind11.h>

#include <isce3/focus/RangeComp.h>

void addbinding(pybind11::enum_<isce3::focus::RangeComp::Mode>&);
void addbinding(pybind11::class_<isce3::focus::RangeComp>&);
