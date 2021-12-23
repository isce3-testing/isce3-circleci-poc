#pragma once

#include <cufft.h>

#include <isce3/except/Error.h>

namespace isce3 { namespace cuda { namespace except {

using namespace isce3::except;

/** CudaError provide the same information as BaseError,
 *  and also retains the original error code.*/
template<class T>
struct CudaError : RuntimeError {
    const T err;
    CudaError(const SrcInfo& info, const T err);
};

// Instantiate the for when the error code is a cudaError_t
// In this case can get a useful error message via cudaGetErrorString
template<>
CudaError<cudaError_t>::CudaError(const SrcInfo& info, const cudaError_t err);

template<>
CudaError<cufftResult>::CudaError(const SrcInfo& info, const cufftResult err);

template<class T>
CudaError<T>::CudaError(const SrcInfo& info, const T err)
    : err(err),
      RuntimeError(info, std::string("cudaError ") + std::to_string(err))
{}

}}} // namespace isce3::cuda::except

template<class T>
static inline void checkCudaErrorsImpl(
        const isce3::except::SrcInfo& info, T err)
{
    if (err)
        throw isce3::cuda::except::CudaError<T>(info, err);
}

/* Explicit instantiation for cufft calls,
 * since we need to compare against cufftResult */
template<>
inline void checkCudaErrorsImpl<cufftResult>(
        const isce3::except::SrcInfo& info, cufftResult err)
{
    if (err != CUFFT_SUCCESS)
        throw isce3::cuda::except::CudaError<cufftResult>(info, err);
}

// Check errors from kernel launches and, if NDEBUG is not defined,
// synchronize and check for execution errors
inline void checkCudaAsyncErrorsImpl(const isce3::except::SrcInfo& info)
{
    checkCudaErrorsImpl(info, cudaPeekAtLastError());
#ifndef NDEBUG
    checkCudaErrorsImpl(info, cudaDeviceSynchronize());
#endif
}

// Wrapper to pass file name, line number, and function name
#define checkCudaErrors(val) checkCudaErrorsImpl(ISCE_SRCINFO(), val)

// Wrapper to pass file name, line number, and function name
#define checkCudaAsyncErrors() checkCudaAsyncErrorsImpl(ISCE_SRCINFO())
