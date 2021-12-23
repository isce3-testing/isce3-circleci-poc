#pragma once

#include <pybind11/pybind11.h>

#include <isce3/geometry/Geo2rdr.h>

void addbinding(pybind11::class_<isce3::geometry::Geo2rdr>&);
void addbinding_geo2rdr(pybind11::module& m);
