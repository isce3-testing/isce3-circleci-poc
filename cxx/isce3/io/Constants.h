//-*- C++ -*-

#pragma once

#include <gdal_priv.h>
#include <string>
#include <unordered_map>

#include "gdal/detail/GDALDataTypeUtil.h"

//! The isce namespace
namespace isce3 {
//! The isce3::io namespace
namespace io {

// Constants for Raster class
/// Default GDAL driver used by Raster for creation
const std::string defaultGDALDriver = "VRT";
/// Default GDAL data type used by Raster for creation
const GDALDataType defaultGDALDataType = GDT_Float32;

template<typename T>
auto asGDT = gdal::detail::Type2GDALDataType<T>::datatype;
} // namespace io
} // namespace isce3
