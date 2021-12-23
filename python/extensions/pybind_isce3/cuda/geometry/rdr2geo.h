#pragma once

#include <pybind11/pybind11.h>

#include <isce3/cuda/geometry/Topo.h>

void addbinding(pybind11::class_<isce3::cuda::geometry::Topo>&);
