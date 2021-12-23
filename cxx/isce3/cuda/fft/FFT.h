#pragma once

#include "FFTPlan.h"

namespace isce3 { namespace cuda { namespace fft {

/**
 * Create a re-useable 1-D forward FFT plan.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] n Transform size
 * \returns Forward FFT plan
 */
template<typename T>
FwdFFTPlan<T> planfft1d(thrust::complex<T>* out, thrust::complex<T>* in, int n);

/** \copydoc planfft1d(thrust::complex<T> * out, thrust::complex<T> * in, int n)
 */
template<typename T>
FwdFFTPlan<T> planfft1d(thrust::complex<T>* out, T* in, int n);

/**
 * Create a re-useable 1-D forward FFT plan for 2-D data.
 *
 * The data is expected to be in row-major format. \p axis = 0 corresponds to
 * a transform along columns, \p axis = 1 corresponds to a row-wise transform.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 * \param[in] axis Axis over which to compute the FFT
 * \returns Forward FFT plan
 */
template<typename T>
FwdFFTPlan<T> planfft1d(thrust::complex<T>* out, thrust::complex<T>* in,
        const int (&dims)[2], int axis);

/** \copydoc planfft1d(thrust::complex<T> * out, thrust::complex<T> * in, const
 * int (&dims)[2], int axis) */
template<typename T>
FwdFFTPlan<T> planfft1d(
        thrust::complex<T>* out, T* in, const int (&dims)[2], int axis);

/**
 * Create a re-useable 2-D forward FFT plan.
 *
 * The data is expected to be in row-major format.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 * \returns Forward FFT plan
 */
template<typename T>
FwdFFTPlan<T> planfft2d(
        thrust::complex<T>* out, thrust::complex<T>* in, const int (&dims)[2]);

/** \copydoc planfft2d(thrust::complex<T> * out, thrust::complex<T> * in, const
 * int (&dims)[2]) */
template<typename T>
FwdFFTPlan<T> planfft2d(thrust::complex<T>* out, T* in, const int (&dims)[2]);

/**
 * Create a re-useable 1-D inverse FFT plan.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] n Transform size
 * \returns Inverse FFT plan
 */
template<typename T>
InvFFTPlan<T> planifft1d(
        thrust::complex<T>* out, thrust::complex<T>* in, int n);

/** \copydoc planifft1d(thrust::complex<T> * out, thrust::complex<T> * in, int
 * n) */
template<typename T>
InvFFTPlan<T> planifft1d(T* out, thrust::complex<T>* in, int n);

/**
 * Create a re-useable 1-D inverse FFT plan for 2-D data.
 *
 * The data is expected to be in row-major format. \p axis = 0 corresponds to
 * a transform along columns, \p axis = 1 corresponds to a row-wise transform.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 * \param[in] axis Axis over which to compute the FFT
 * \returns Inverse FFT plan
 */
template<typename T>
InvFFTPlan<T> planifft1d(thrust::complex<T>* out, thrust::complex<T>* in,
        const int (&dims)[2], int axis);

/** \copydoc planifft1d(thrust::complex<T> * out, thrust::complex<T> * in, const
 * int (&dims)[2], int axis) */
template<typename T>
InvFFTPlan<T> planifft1d(
        T* out, thrust::complex<T>* in, const int (&dims)[2], int axis);

/**
 * Create a re-useable 2-D inverse FFT plan.
 *
 * The data is expected to be in row-major format.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 * \returns Inverse FFT plan
 */
template<typename T>
InvFFTPlan<T> planifft2d(
        thrust::complex<T>* out, thrust::complex<T>* in, const int (&dims)[2]);

/** \copydoc planifft2d(thrust::complex<T> * out, thrust::complex<T> * in, const
 * int (&dims)[2]) */
template<typename T>
InvFFTPlan<T> planifft2d(T* out, thrust::complex<T>* in, const int (&dims)[2]);

/**
 * Compute the 1-D forward FFT.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] n Transform size
 */
template<typename T>
void fft1d(thrust::complex<T>* out, thrust::complex<T>* in, int n);

/** \copydoc fft1d(thrust::complex<T> * out, thrust::complex<T> * in, int n) */
template<typename T>
void fft1d(thrust::complex<T>* out, T* in, int n);

/**
 * Compute the 1-D forward FFT on 2-D data.
 *
 * The data is expected to be in row-major format. \p axis = 0 corresponds to
 * a transform along columns, \p axis = 1 corresponds to a row-wise transform.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 * \param[in] axis Axis over which to compute the FFT
 */
template<typename T>
void fft1d(thrust::complex<T>* out, thrust::complex<T>* in,
        const int (&dims)[2], int axis);

/** \copydoc fft1d(thrust::complex<T> * out, thrust::complex<T> * in, const int
 * (&dims)[2], int axis) */
template<typename T>
void fft1d(thrust::complex<T>* out, T* in, const int (&dims)[2], int axis);

/**
 * Compute the 2-D forward FFT.
 *
 * The data is expected to be in row-major format.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 */
template<typename T>
void fft2d(
        thrust::complex<T>* out, thrust::complex<T>* in, const int (&dims)[2]);

/** \copydoc fft2d(thrust::complex<T> * out, thrust::complex<T> * in, const int
 * (&dims)[2]) */
template<typename T>
void fft2d(thrust::complex<T>* out, T* in, const int (&dims)[2]);

/**
 * Compute the 1-D inverse FFT.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] n Transform size
 */
template<typename T>
void ifft1d(thrust::complex<T>* out, thrust::complex<T>* in, int n);

/** \copydoc ifft1d(thrust::complex<T> * out, thrust::complex<T> * in, int n) */
template<typename T>
void ifft1d(T* out, thrust::complex<T>* in, int n);

/**
 * Compute the 1-D inverse FFT on 2-D data.
 *
 * The data is expected to be in row-major format. \p axis = 0 corresponds to
 * a transform along columns, \p axis = 1 corresponds to a row-wise transform.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 * \param[in] axis Axis over which to compute the FFT
 */
template<typename T>
void ifft1d(thrust::complex<T>* out, thrust::complex<T>* in,
        const int (&dims)[2], int axis);

/** \copydoc ifft1d(thrust::complex<T> * out, thrust::complex<T> * in, const int
 * (&dims)[2], int axis) */
template<typename T>
void ifft1d(T* out, thrust::complex<T>* in, const int (&dims)[2], int axis);

/**
 * Compute the 2-D inverse FFT.
 *
 * The data is expected to be in row-major format.
 *
 * \param[out] out Output buffer
 * \param[in] in Input data
 * \param[in] dims Input/output array shape (nrows, ncols)
 */
template<typename T>
void ifft2d(
        thrust::complex<T>* out, thrust::complex<T>* in, const int (&dims)[2]);

/** \copydoc ifft2d(thrust::complex<T> * out, thrust::complex<T> * in, const int
 * (&dims)[2]) */
template<typename T>
void ifft2d(T* out, thrust::complex<T>* in, int (&dims)[2]);

}}} // namespace isce3::cuda::fft

#define ISCE_CUDA_FFT_FFT_ICC
#include "FFT.icc"
#undef ISCE_CUDA_FFT_FFT_ICC
