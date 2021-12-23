#pragma once

#include <pybind11/pybind11.h>

#include <isce3/product/GeoGridParameters.h>

void addbinding(pybind11::class_<isce3::product::GeoGridParameters>&);
void addbinding_bbox_to_geogrid(pybind11::module& m);
