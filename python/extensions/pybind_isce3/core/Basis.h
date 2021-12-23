#pragma once

#include <pybind11/pybind11.h>

#include <isce3/core/Basis.h>

void addbinding(pybind11::class_<isce3::core::Basis>&);
void addbinding_basis(pybind11::module&);
