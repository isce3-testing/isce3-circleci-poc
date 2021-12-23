#pragma once

#include <pybind11/pybind11.h>

#include <isce3/geometry/RTC.h>

using isce3::geometry::rtcAlgorithm;
using isce3::geometry::rtcAreaMode;
using isce3::geometry::rtcInputTerrainRadiometry;
using isce3::geometry::rtcOutputTerrainRadiometry;

void addbinding(
        pybind11::enum_<rtcInputTerrainRadiometry>& pyInputTerrainRadiometry);
void addbinding(
        pybind11::enum_<rtcOutputTerrainRadiometry>& pyOutputTerrainRadiometry);
void addbinding(pybind11::enum_<rtcAlgorithm>& pyRtcAlgorithm);
void addbinding(pybind11::enum_<rtcAreaMode>& pyRtcAreaMode);

void addbinding_apply_rtc(pybind11::module& m);
void addbinding_compute_rtc(pybind11::module& m);
void addbinding_compute_rtc_bbox(pybind11::module& m);
