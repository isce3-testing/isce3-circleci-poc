#include <cuda_runtime.h>
#include <iostream>
#include <stdio.h>
#include <thrust/complex.h>
#include <valarray>

#include <isce3/core/Matrix.h>
#include <isce3/cuda/except/Error.h>

#include "gpuInterpolator.h"

using isce3::cuda::core::gpuInterpolator;
using isce3::cuda::core::gpuSinc2dInterpolator;

/*
   each derived class needs it's own wrapper_d, gpuInterpolator_g...
*/

template<class T>
__host__ gpuSinc2dInterpolator<T>::gpuSinc2dInterpolator(
        int sincLen, int sincSub)
    : kernel_length(sincSub), kernel_width(sincLen), sinc_half(sincLen / 2),
      owner(true)
{
    // Temporary valarray for storing sinc coefficients
    std::valarray<double> filter(0.0, sincSub * sincLen);
    sinc_coef(1.0, sincLen, sincSub, 0.0, 1, filter);

    // Kernel matrix sized to transposed filter coefficients
    Matrix<double> h_kernel;
    h_kernel.resize(sincSub, sincLen);

    // Normalize filter
    for (size_t i = 0; i < sincSub; ++i) {
        // Compute filter sum
        double ssum = 0.0;
        for (size_t j = 0; j < sincLen; ++j) {
            ssum += filter[i + sincSub * j];
        }
        // Normalize the filter coefficients and copy to transposed kernel
        for (size_t j = 0; j < sincLen; ++j) {
            filter[i + sincSub * j] /= ssum;
            h_kernel(i, j) = filter[i + sincSub * j];
        }
    }

    // Malloc device-side memory (this API is host-side only)
    checkCudaErrors(cudaMalloc(&kernel, filter.size() * sizeof(double)));

    // Copy Orbit data to device-side memory and keep device pointer in gpuOrbit
    // object. Device-side copy constructor simply shallow-copies the device
    // pointers when called
    checkCudaErrors(cudaMemcpy(kernel, h_kernel.data(),
            filter.size() * sizeof(double), cudaMemcpyHostToDevice));
}

template<class T>
__global__ void gpuInterpolator_g(gpuSinc2dInterpolator<T> interp, double* x,
        double* y, const T* z, T* value, size_t nx, size_t ny = 0)
{
    /*
     *  GPU kernel to test interpolate() on the device for consistency.
        z := chip
     */
    int i = threadIdx.x;
    value[i] = interp.interpolate(x[i], y[i], z, nx, ny);
}

template<class T>
__host__ void gpuSinc2dInterpolator<T>::interpolate_h(
        const Matrix<double>& truth, Matrix<T>& chip, double start,
        double delta, T* h_z)
{
    /*
     *  CPU-side function to call the corresponding GPU function on a single
     thread for consistency checking truth = indices to interpolate to start,
     delta = unused h_z = output
     */

    // allocate host side memory
    size_t size_input_pts = truth.length() * sizeof(double);
    size_t size_output_pts = truth.length() * sizeof(T);
    double* h_x = (double*) malloc(size_input_pts);
    double* h_y = (double*) malloc(size_input_pts);

    // assign host side inputs
    for (size_t i = 0; i < truth.length(); ++i) {
        h_x[i] = truth(i, 0);
        h_y[i] = truth(i, 1);
    }

    size_t nx = chip.width();
    size_t ny = chip.length();

    // allocate device side memory
    double* d_x;
    checkCudaErrors(cudaMalloc((void**) &d_x, size_input_pts));
    double* d_y;
    checkCudaErrors(cudaMalloc((void**) &d_y, size_input_pts));
    T* d_z;
    checkCudaErrors(cudaMalloc((void**) &d_z, size_output_pts));
    T* d_chip;
    checkCudaErrors(cudaMalloc(
            (T**) &d_chip, chip.length() * chip.width() * sizeof(T)));

    // copy input data
    checkCudaErrors(
            cudaMemcpy(d_x, h_x, size_input_pts, cudaMemcpyHostToDevice));
    checkCudaErrors(
            cudaMemcpy(d_y, h_y, size_input_pts, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(d_chip, chip.data(),
            chip.length() * chip.width() * sizeof(T), cudaMemcpyHostToDevice));

    // launch!
    int n_threads = truth.length();
    gpuInterpolator_g<T>
            <<<1, n_threads>>>(*this, d_x, d_y, d_chip, d_z, nx, ny);

    // copy device results to host
    checkCudaErrors(
            cudaMemcpy(h_z, d_z, size_output_pts, cudaMemcpyDeviceToHost));

    // free memory
    checkCudaErrors(cudaFree(d_x));
    checkCudaErrors(cudaFree(d_y));
    checkCudaErrors(cudaFree(d_z));
    checkCudaErrors(cudaFree(d_chip));
}

template<class T>
__host__ void gpuSinc2dInterpolator<T>::sinc_coef(double beta,
        double relfiltlen, int decfactor, double pedestal, int weight,
        std::valarray<double>& filter)
{

    int filtercoef = int(filter.size());
    double wgthgt = (1.0 - pedestal) / 2.0;
    double soff = (filtercoef - 1.) / 2.;

    double wgt, s, fct;
    for (int i = 0; i < filtercoef; i++) {
        wgt = (1. - wgthgt) + (wgthgt * cos((M_PI * (i - soff)) / soff));
        s = (floor(i - soff) * beta) / (1. * decfactor);
        fct = ((s != 0.) ? (sin(M_PI * s) / (M_PI * s)) : 1.);
        filter[i] = ((weight == 1) ? (fct * wgt) : fct);
    }
}

template<class T>
__device__ T gpuSinc2dInterpolator<T>::interpolate(
        double x, double y, const T* chip, size_t nx, size_t ny)
{
    /*
    definitions with respect to ResampSlc interpolate and sinc_eval_2d
    x   := fracAz
    y   := fracRg
    z   := chip
    nx  := chip length
    ny  := chip width
    */
    // Initialize return value
    T interp_val(0.0);

    // Separate interpolation coordinates into integer and fractional components
    const int ix = __double2int_rd(x);
    const int iy = __double2int_rd(y);
    const double frpx = x - ix;
    const double frpy = y - iy;

    // Check edge conditions
    bool x_in_bounds =
            ((ix >= (sinc_half - 1)) && (ix <= (ny - sinc_half - 1)));
    bool y_in_bounds =
            ((iy >= (sinc_half - 1)) && (iy <= (nx - sinc_half - 1)));
    if (x_in_bounds && y_in_bounds) {

        // Modify integer interpolation coordinates for sinc evaluation
        const int intpx = ix + sinc_half;
        const int intpy = iy + sinc_half;

        // Get nearest kernel indices
        int ifracx = min(max(0, int(frpx * kernel_length)), kernel_length - 1);
        int ifracy = min(max(0, int(frpy * kernel_length)), kernel_length - 1);

        // Compute weighted sum
        for (int i = 0; i < kernel_width; i++) {
            for (int j = 0; j < kernel_width; j++) {
                interp_val += chip[(intpy - i) * nx + intpx - j] *
                              T(kernel[ifracy * kernel_width + i]) *
                              T(kernel[ifracx * kernel_width + j]);
            }
        }
    }
    // Done
    return interp_val;
}

template<class T>
__host__ __device__ gpuSinc2dInterpolator<T>::~gpuSinc2dInterpolator()
{
#ifndef __CUDA_ARCH__
    if (owner)
        checkCudaErrors(cudaFree(kernel));
#endif
}

// Explicit instantiation
template class gpuSinc2dInterpolator<float>;
template class gpuSinc2dInterpolator<double>;
template class gpuSinc2dInterpolator<thrust::complex<double>>;
template class gpuSinc2dInterpolator<thrust::complex<float>>;

template __global__ void gpuInterpolator_g<double>(
        gpuSinc2dInterpolator<double> interp, double* x, double* y,
        const double* z, double* value, size_t nx, size_t ny);
template __global__ void gpuInterpolator_g<thrust::complex<double>>(
        gpuSinc2dInterpolator<thrust::complex<double>> interp, double* x,
        double* y, const thrust::complex<double>* z,
        thrust::complex<double>* value, size_t nx, size_t ny);
template __global__ void gpuInterpolator_g<thrust::complex<float>>(
        gpuSinc2dInterpolator<thrust::complex<float>> interp, double* x,
        double* y, const thrust::complex<float>* z,
        thrust::complex<float>* value, size_t nx, size_t ny);
