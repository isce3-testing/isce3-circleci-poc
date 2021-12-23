#pragma once

#include <pybind11/pybind11.h>

#include <isce3/core/Linspace.h>

template<typename T>
void addbinding(pybind11::class_<isce3::core::Linspace<T>>&);
