#include "Interp1d.h"

#include <pybind11/complex.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <valarray>

#include <isce3/core/Interp1d.h>
#include <isce3/core/Kernels.h>
#include <isce3/except/Error.h>

namespace py = pybind11;

using namespace isce3::core;
using isce3::except::RuntimeError;

template<typename TK, typename TD>
static py::object interp_duckt(
        const Kernel<TK>& kernel, py::buffer_info& info, py::object t)
{
    TD* data = static_cast<TD*>(info.ptr);
    int stride = info.strides[0] / sizeof(TD);
    auto n = info.shape[0];
    if (py::isinstance<py::float_>(t)) {
        return py::cast(interp1d(kernel, data, n, stride, py::float_(t)));
    } else if (py::isinstance<py::array_t<double>>(t)) {
        auto ta = py::array_t<double>(t).unchecked<1>();
        std::valarray<TD> out(ta.size());
        for (size_t i = 0; i < ta.size(); ++i) {
            out[i] = interp1d(kernel, data, n, stride, ta(i));
        }
        return py::cast(out, py::return_value_policy::take_ownership);
    }
    throw RuntimeError(ISCE_SRCINFO(),
            "interp1d expects time is float or numpy.float64 array");
}

template<typename T>
static py::object interp_duckbuf(
        Kernel<T>& kernel, py::buffer buf, py::object t)
{
    py::buffer_info info = buf.request();
    using C8 = std::complex<float>;
    using C16 = std::complex<double>;
    if (info.ndim != 1) {
        throw RuntimeError(ISCE_SRCINFO(), "data buffer must be 1-D");
    }
    if (info.format == py::format_descriptor<float>::format()) {
        return interp_duckt<T, float>(kernel, info, t);
    } else if (info.format == py::format_descriptor<double>::format()) {
        return interp_duckt<T, double>(kernel, info, t);
    } else if (info.format == py::format_descriptor<C8>::format()) {
        return interp_duckt<T, C8>(kernel, info, t);
    } else if (info.format == py::format_descriptor<C16>::format()) {
        return interp_duckt<T, C16>(kernel, info, t);
    }
    throw RuntimeError(ISCE_SRCINFO(), "Unsupported types for interp1d");
}

void addbinding_interp1d(py::module& m)
{
    m.def(
            "interp1d",
            [](py::object pyKernel, py::buffer buf, py::object t) {
                if (py::isinstance<Kernel<float>>(pyKernel)) {
                    auto kernel = pyKernel.cast<Kernel<float>*>();
                    return interp_duckbuf(*kernel, buf, t);
                } else if (py::isinstance<Kernel<double>>(pyKernel)) {
                    auto kernel = pyKernel.cast<Kernel<double>*>();
                    return interp_duckbuf(*kernel, buf, t);
                }
                throw RuntimeError(
                        ISCE_SRCINFO(), "Expected Kernel or KernelF32");
            },
            R"(
        Interpolate a 1D sequence `data` at `time` using `kernel`.  The `time`
        units are sample numbers (starting at zero), and `time` may be a
        scalar or an array.
    )",
            py::arg("kernel"), py::arg("data"), py::arg("time"));
}
