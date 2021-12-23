#include "Error.h"

using namespace isce3::except;

// generic error message
std::string errmsg(const SrcInfo& info)
{
    return "Error in file " + std::string(info.file) + ", line " +
           std::to_string(info.line) + ", function " + info.func;
}

// message with generic prefix
std::string errmsg(const SrcInfo& info, std::string msg)
{
    return errmsg(info) + ": " + msg;
}

template<typename T>
Error<T>::Error(const SrcInfo& info) : T(errmsg(info)), info(info)
{}

template<typename T>
Error<T>::Error(const SrcInfo& info, std::string msg)
    : T(errmsg(info, msg)), info(info)
{}

template class isce3::except::Error<std::domain_error>;
template class isce3::except::Error<std::invalid_argument>;
template class isce3::except::Error<std::length_error>;
template class isce3::except::Error<std::out_of_range>;
template class isce3::except::Error<std::overflow_error>;
template class isce3::except::Error<std::runtime_error>;
