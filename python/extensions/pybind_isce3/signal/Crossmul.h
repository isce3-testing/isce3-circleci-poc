#pragma once

#include <pybind11/pybind11.h>

#include <isce3/signal/Crossmul.h>

void addbinding(pybind11::class_<isce3::signal::Crossmul>&);
