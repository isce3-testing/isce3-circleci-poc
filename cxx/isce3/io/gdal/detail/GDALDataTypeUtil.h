#pragma once

#include <complex>
#include <cstdint>
#include <gdal_priv.h>
#include <type_traits>

// forward declare thrust::complex
namespace thrust {
template<typename>
struct complex;
}

namespace isce3 { namespace io { namespace gdal { namespace detail {

template<GDALDataType DataType>
struct GDT {
    static constexpr GDALDataType datatype = DataType;
};

template<typename T>
struct Type2GDALDataType : public GDT<GDT_Unknown> {};

// char is always a single byte
template<>
struct Type2GDALDataType<char> : public GDT<GDT_Byte> {};
template<>
struct Type2GDALDataType<signed char> : public GDT<GDT_Byte> {};
template<>
struct Type2GDALDataType<unsigned char> : public GDT<GDT_Byte> {};

// fixed-size integer types
template<>
struct Type2GDALDataType<std::int16_t> : public GDT<GDT_Int16> {};
template<>
struct Type2GDALDataType<std::int32_t> : public GDT<GDT_Int32> {};

// fixed-size unsigned integer types
template<>
struct Type2GDALDataType<std::uint16_t> : public GDT<GDT_UInt16> {};
template<>
struct Type2GDALDataType<std::uint32_t> : public GDT<GDT_UInt32> {};

// floating point types
template<>
struct Type2GDALDataType<float> : public GDT<GDT_Float32> {};
template<>
struct Type2GDALDataType<double> : public GDT<GDT_Float64> {};

// complex floating point types
template<>
struct Type2GDALDataType<std::complex<float>> : public GDT<GDT_CFloat32> {};
template<>
struct Type2GDALDataType<std::complex<double>> : public GDT<GDT_CFloat64> {};

// thrust::complex floating point types
template<>
struct Type2GDALDataType<thrust::complex<float>> : public GDT<GDT_CFloat32> {};
template<>
struct Type2GDALDataType<thrust::complex<double>> : public GDT<GDT_CFloat64> {};

constexpr std::size_t getSize(GDALDataType datatype)
{
    switch (datatype) {
    case GDT_Byte: return sizeof(unsigned char);
    case GDT_UInt16: return sizeof(std::uint16_t);
    case GDT_Int16: return sizeof(std::int16_t);
    case GDT_UInt32: return sizeof(std::uint32_t);
    case GDT_Int32: return sizeof(std::int32_t);
    case GDT_Float32: return sizeof(float);
    case GDT_Float64: return sizeof(double);
    case GDT_CFloat32: return sizeof(std::complex<float>);
    case GDT_CFloat64: return sizeof(std::complex<double>);
    default: return 0;
    }
}

}}}} // namespace isce3::io::gdal::detail
