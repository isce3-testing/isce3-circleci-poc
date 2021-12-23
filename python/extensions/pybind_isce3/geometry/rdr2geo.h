#pragma once

#include <pybind11/pybind11.h>

#include <isce3/geometry/Topo.h>

void addbinding_rdr2geo(pybind11::module& m);
void addbinding(pybind11::class_<isce3::geometry::Topo>&);
