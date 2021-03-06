#include <cuda_runtime.h>
#include <vector>
#include <isce3/core/Constants.h>
#include <isce3/core/Poly2d.h>
#include "gpuPoly2d.h"
#include <stdio.h>

using isce3::cuda::core::gpuPoly2d;
using isce3::core::Poly2d;
using std::vector;


// Advanced "copy" constructor to handle deep-copying of Poly2d data (only callable by host). Owner
// member variable indicates that only the host-side copy of the gpuPoly2d can handle freeing the
// memory (device-side copy constructor for gpuPoly2d sets owner to false)
__host__ gpuPoly2d::gpuPoly2d(const Poly2d &poly) :
    xOrder(poly.xOrder),
    yOrder(poly.yOrder),
    xMean(poly.xMean),
    yMean(poly.yMean),
    xNorm(poly.xNorm),
    yNorm(poly.yNorm),
    owner(true)
{

    const int n_coeffs = poly.coeffs.size();

    // Malloc device-side memory (this API is host-side only)
    cudaMalloc(&coeffs, n_coeffs*sizeof(double));

    // Copy OrPoly2d data to device-side memory and keep device pointer in gpuOrPoly2d object. Device-side
    // copy constructor simply shallow-copies the device pointers when called
    cudaMemcpy(coeffs, poly.coeffs.data(), n_coeffs*sizeof(double), cudaMemcpyHostToDevice);
}


// Both the host-side and device-side copies of the gpuPoly2d will call the destructor, so we have to
// implement a way of having an arbitrary copy on host OR device determine when to free the memory
// (since only the original host-side copy should free)
gpuPoly2d::~gpuPoly2d() {
    if (owner) {
        cudaFree(coeffs);
    }
}

__device__ double gpuPoly2d::eval(double y, double x) const {

    double xval = (x - xMean) / xNorm;
    double yval = (y - yMean) / yNorm;

    double scalex;
    double scaley = 1.;
    double val = 0.;
    for (int i=0; i<=yOrder; i++,scaley*=yval) {
        scalex = 1.;
        for (int j=0; j<=xOrder; j++,scalex*=xval) {
            val += scalex * scaley * coeffs[IDX1D(i,j,xOrder+1)];
        }
    }

    return val;
}

__global__ void eval_d(gpuPoly2d p, double y, double x, double *val)
{
    *val = p.eval(y, x);
}

__host__ double gpuPoly2d::eval_h(double y, double x)
{
    double *val_d;
    double val_h;
    // use unified memory?
    cudaMalloc((double**)&val_d, sizeof(double));
    dim3 grid(1), block(1);
    eval_d<<<grid,block>>>(*this, y, x, val_d);
    cudaMemcpy(&val_h, val_d, sizeof(double), cudaMemcpyDeviceToHost);
    cudaFree(val_d);
    return val_h;
}
