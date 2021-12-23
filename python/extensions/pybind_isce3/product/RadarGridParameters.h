#pragma once

#include <pybind11/pybind11.h>

#include <isce3/product/RadarGridParameters.h>

void addbinding(pybind11::class_<isce3::product::RadarGridParameters>&);
