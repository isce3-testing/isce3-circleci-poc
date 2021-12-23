#pragma once

#include <pybind11/pybind11.h>

#include <isce3/geocode/GeocodeCov.h>

template<typename T>
void addbinding(pybind11::class_<isce3::geocode::Geocode<T>>&);
void addbinding(pybind11::enum_<isce3::geocode::geocodeMemoryMode>&);
void addbinding(pybind11::enum_<isce3::geocode::geocodeOutputMode>&);
