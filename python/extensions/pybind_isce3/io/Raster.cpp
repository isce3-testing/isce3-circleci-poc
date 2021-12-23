#include "Raster.h"

#include <gdal_priv.h>
#include <iostream>
#include <pybind11/stl.h>
#include <string>
#include <vector>

#include <isce3/except/Error.h>
#include <isce3/io/IH5Dataset.h>

namespace py = pybind11;

using isce3::io::Raster;

auto py2CxxDtype = [](const int dtype) {
    switch (dtype) {
    case 1: return GDT_Byte;
    case 2: return GDT_UInt16;
    case 3: return GDT_Int16;
    case 4: return GDT_UInt32;
    case 5: return GDT_Int32;
    case 6: return GDT_Float32;
    case 7: return GDT_Float64;
    case 10: return GDT_CFloat32;
    case 11: return GDT_CFloat64;
    default: break;
    }
    throw isce3::except::RuntimeError(
            ISCE_SRCINFO(), "unsupported GDAL datatype");
};

auto cxx2PyDtype = [](const int dtype) {
    switch (dtype) {
    case GDT_Byte: return 1;
    case GDT_UInt16: return 2;
    case GDT_Int16: return 3;
    case GDT_UInt32: return 4;
    case GDT_Int32: return 5;
    case GDT_Float32: return 6;
    case GDT_Float64: return 7;
    case GDT_CFloat32: return 10;
    case GDT_CFloat64: return 11;
    default: break;
    }
    throw isce3::except::RuntimeError(
            ISCE_SRCINFO(), "unsupported GDAL datatype");
};

void addbinding(py::class_<Raster>& pyRaster)
{
    pyRaster
            // update and read-only (default) mode
            .def(py::init([](const std::string& path, bool update) {
                if (update) {
                    // register IH5
                    if (path.rfind("IH5:::", 0) == 0) {
                        isce3::io::GDALRegister_IH5();
                    }
                    return std::make_unique<Raster>(path, GA_Update);
                } else {
                    return std::make_unique<Raster>(path);
                }
            }),
                    "Open raster in update or read-only (default) mode",
                    py::arg("path"), py::arg("update") = false)
            // dataset constructor
            .def(py::init([](const std::string& path, int width, int length,
                                  int num_bands, int dtype,
                                  const std::string driver_name) {
                auto gd_dtype = py2CxxDtype(dtype);
                return std::make_unique<Raster>(
                        path, width, length, num_bands, gd_dtype, driver_name);
            }),
                    "Create a raster dataset", py::arg("path"),
                    py::arg("width"), py::arg("length"), py::arg("num_bands"),
                    py::arg("dtype"), py::arg("driver_name"))
            // multiband constructor
            .def(py::init([](const std::string& path,
                                  std::vector<Raster> raster_list) {
                return std::make_unique<Raster>(path, raster_list);
            }),
                    "Create a VRT raster dataset from list of rasters",
                    py::arg("path"), py::arg("raster_list"))
            .def(
                    "close_dataset",
                    [](Raster& self) {
                        if (self.dataset_owner()) {
                            auto gdal_ds = self.dataset();
                            delete gdal_ds;
                        }
                        self.dataset(nullptr);
                    },
                    R"(
            Close the dataset.

            Decrements the reference count of the underlying `GDALDataset`, which,
            if this was the last open instance, causes the dataset to be closed
            and any cached changes to be flushed to disk.

            This invalidates the `Raster` instance -- it cannot be used after closing
            the underlying dataset.
            )")
            .def(py::init([](std::uintptr_t py_ds_ptr) {
                auto gdal_ds = reinterpret_cast<GDALDataset*>(py_ds_ptr);
                return std::make_unique<Raster>(gdal_ds, false);
            }),
                    "Create a raster from Python GDAlDataset",
                    py::arg("py_ds_ptr"))
            .def_property_readonly("width", &Raster::width)
            .def_property_readonly("length", &Raster::length)
            .def_property_readonly("num_bands", &Raster::numBands)
            .def_property_readonly("dx", &Raster::dx)
            .def_property_readonly("dy", &Raster::dy)
            .def_property_readonly(
                    "access", [](Raster& self) { return self.access(); })
            .def_property_readonly(
                    "readonly", [](Raster& self) { return self.access() == 0; })
            .def("get_geotransform",
                    [](Raster& self) {
                        std::vector<double> transform(6);
                        self.getGeoTransform(transform);
                        return transform;
                    })
            .def("set_geotransform",
                    [](Raster& self, std::vector<double> transform) {
                        self.setGeoTransform(transform);
                    })
            .def(
                    "datatype",
                    [](Raster& self, int i) {
                        return cxx2PyDtype(self.dtype(i));
                    },
                    py::arg("band") = 1)
            .def("get_epsg", &Raster::getEPSG)
            .def("set_epsg", &Raster::setEPSG);
}

// end of file
