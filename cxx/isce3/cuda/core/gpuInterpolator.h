//
// Author: Liang Yu
// Copyright 2018
//

#pragma once

#include <isce3/core/forward.h>

#include <complex>
#include <stdio.h>

#include <isce3/core/Common.h>

using isce3::core::Matrix;

/** base interpolator is an abstract base class */
namespace isce3 { namespace cuda { namespace core {
template<class U>
class gpuInterpolator {
public:
    CUDA_HOSTDEV gpuInterpolator() {}
    CUDA_HOSTDEV virtual ~gpuInterpolator() {}
    CUDA_DEV virtual U interpolate(
            double, double, const U*, size_t, size_t) = 0;
};

/** gpuBilinearInterpolator class derived from abstract gpuInterpolator class */
template<class U>
class gpuBilinearInterpolator : public isce3::cuda::core::gpuInterpolator<U> {
public:
    CUDA_HOSTDEV gpuBilinearInterpolator() {};
    CUDA_DEV U interpolate(double, double, const U*, size_t, size_t);
    CUDA_HOST void interpolate_h(
            const Matrix<double>&, Matrix<U>&, double, double, U*);
};

/** gpuBicubicInterpolator class derived from abstract gpuInterpolator class */
template<class U>
class gpuBicubicInterpolator : public isce3::cuda::core::gpuInterpolator<U> {
public:
    CUDA_HOSTDEV gpuBicubicInterpolator() {};
    CUDA_DEV U interpolate(double, double, const U*, size_t, size_t);
    CUDA_HOST void interpolate_h(
            const Matrix<double>&, Matrix<U>&, double, double, U*);
};

/** gpuSpline2dInterpolator class derived from abstract gpuInterpolator class */
template<class U>
class gpuSpline2dInterpolator : public isce3::cuda::core::gpuInterpolator<U> {
protected:
    size_t _order;

public:
    CUDA_HOSTDEV gpuSpline2dInterpolator(size_t order) : _order(order) {};
    CUDA_DEV U interpolate(double, double, const U*, size_t, size_t);
    CUDA_HOST void interpolate_h(
            const Matrix<double>&, Matrix<U>&, double, double, U*);
};

/** gpuSinc2dInterpolator class derived from abstract gpuInterpolator class */
template<class U>
class gpuSinc2dInterpolator : public isce3::cuda::core::gpuInterpolator<U> {
protected:
    double* kernel;
    int kernel_length, kernel_width,
            sinc_half; // filter dimension idec=length, ilen=width
    int intpx, intpy;  // sub-chip/image bounds? unclear...
    // True if initialized from host, false if copy-constructed from
    // gpuSinc2dInterpolator on device
    bool owner;

public:
    CUDA_HOSTDEV gpuSinc2dInterpolator() {};
    CUDA_HOST gpuSinc2dInterpolator(int sincLen, int sincSub);
    CUDA_HOSTDEV gpuSinc2dInterpolator(const gpuSinc2dInterpolator& i)
        : kernel(i.kernel), kernel_length(i.kernel_length),
          kernel_width(i.kernel_width), sinc_half(i.sinc_half), intpx(i.intpx),
          intpy(i.intpy), owner(false) {};
    CUDA_HOSTDEV ~gpuSinc2dInterpolator();
    CUDA_HOST void sinc_coef(
            double, double, int, double, int, std::valarray<double>&);
    CUDA_DEV U interpolate(double, double, const U*, size_t, size_t);
    CUDA_HOST void interpolate_h(
            const Matrix<double>&, Matrix<U>&, double, double, U*);
};

/** gpuNearestNeighborInterpolator class derived from abstract gpuInterpolator
 * class */
template<class T>
class gpuNearestNeighborInterpolator :
    public isce3::cuda::core::gpuInterpolator<T> {
public:
    CUDA_HOSTDEV gpuNearestNeighborInterpolator() {};
    CUDA_DEV T interpolate(
            double x, double y, const T* z, size_t nx, size_t ny = 0);
};

}}} // namespace isce3::cuda::core
