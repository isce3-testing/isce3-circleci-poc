// -*- C++ -*-
// -*- coding: utf-8 -*-
//
// Author: Heresh Fattahi
// Copyright 2019-
//

#include <cmath>
#include <complex>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include <gtest/gtest.h>

#include <isce3/io/Raster.h>
#include <isce3/signal/Covariance.h>
#include <isce3/signal/Crossmul.h>

// To create test data
void createTestData();

TEST(Covariance, DualpolRun)
{

    // consider only one pixel
    size_t width = 10;
    size_t length = 10;
    size_t rngLooks = 1;
    size_t azLooks = 1;
    size_t widthLooked = width / rngLooks;
    size_t lengthLooked = length / azLooks;

    createTestData();

    // rasters for two SLC polarizations
    isce3::io::Raster slcHH("hh.vrt");
    isce3::io::Raster slcHV("hv.vrt");

    std::map<std::string, isce3::io::Raster> slcList = {
            {"hh", slcHH}, {"hv", slcHV}};

    isce3::io::Raster c_hh_hh(
            "cov_hh_hh.vrt", widthLooked, lengthLooked, 1, GDT_CFloat32, "VRT");
    isce3::io::Raster c_hh_hv(
            "cov_hh_hv.vrt", widthLooked, lengthLooked, 1, GDT_CFloat32, "VRT");
    isce3::io::Raster c_hv_hv(
            "cov_hv_hv.vrt", widthLooked, lengthLooked, 1, GDT_CFloat32, "VRT");

    std::map<std::pair<std::string, std::string>, isce3::io::Raster> covList = {
            {std::make_pair("hh", "hh"), c_hh_hh},
            {std::make_pair("hh", "hv"), c_hh_hv},
            {std::make_pair("hv", "hv"), c_hv_hv}};

    std::cout << "define covariance obj" << std::endl;
    isce3::signal::Covariance<std::complex<float>> covarianceObj;
    isce3::signal::Crossmul crsmul;

    std::cout << "end" << std::endl;

    covarianceObj.covariance(slcList, covList, rngLooks, azLooks);
}

TEST(Covariance, DualpolCheck)
{

    // read rasters for two SLC polarizations
    isce3::io::Raster slcHH("hh.vrt");
    isce3::io::Raster slcHV("hv.vrt");

    size_t length = slcHH.length();
    size_t width = slcHH.width();

    std::valarray<std::complex<float>> shh(length * width);
    std::valarray<std::complex<float>> shv(length * width);

    slcHH.getBlock(shh, 0, 0, width, length);
    slcHV.getBlock(shv, 0, 0, width, length);

    // expected covarinace values
    std::valarray<std::complex<float>> expected_c_hh_hh(length * width);
    std::valarray<std::complex<float>> expected_c_hh_hv(length * width);
    std::valarray<std::complex<float>> expected_c_hv_hv(length * width);

    // compute expected covariance values
    for (size_t i = 0; i < length * width; i++) {

        expected_c_hh_hh[i] = shh[i] * std::conj(shh[i]);
        expected_c_hh_hv[i] = shh[i] * std::conj(shv[i]);
        expected_c_hv_hv[i] = shv[i] * std::conj(shv[i]);
    }

    // raster of the computed covariance values
    isce3::io::Raster c_hh_hh_raster("cov_hh_hh.vrt");
    isce3::io::Raster c_hh_hv_raster("cov_hh_hv.vrt");
    isce3::io::Raster c_hv_hv_raster("cov_hv_hv.vrt");

    // valarrays to read the computed covariance values
    std::valarray<std::complex<float>> c_hh_hh(length * width);
    std::valarray<std::complex<float>> c_hh_hv(length * width);
    std::valarray<std::complex<float>> c_hv_hv(length * width);

    // read the computed covariance values
    c_hh_hh_raster.getBlock(c_hh_hh, 0, 0, width, length);
    c_hh_hv_raster.getBlock(c_hh_hv, 0, 0, width, length);
    c_hv_hv_raster.getBlock(c_hv_hv, 0, 0, width, length);

    double tol = 1e-5;
    for (size_t i = 0; i < length * width; i++) {

        ASSERT_NEAR(std::arg(c_hh_hh[i]), std::arg(expected_c_hh_hh[i]), tol);
        ASSERT_NEAR(std::arg(c_hh_hv[i]), std::arg(expected_c_hh_hv[i]), tol);
        ASSERT_NEAR(std::arg(c_hv_hv[i]), std::arg(expected_c_hv_hv[i]), tol);
    }
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

void createTestData()
{

    size_t width = 10;
    size_t length = 10;

    // make rasters for two SLC polarizations
    isce3::io::Raster slcHH("hh.vrt", width, length, 1, GDT_CFloat32, "VRT");
    isce3::io::Raster slcHV("hv.vrt", width, length, 1, GDT_CFloat32, "VRT");

    // consider some values for the two slcs

    std::valarray<std::complex<float>> shh(length * width);
    std::valarray<std::complex<float>> shv(length * width);

    for (size_t i = 0; i < length * width; ++i) {

        shh[i] = std::complex<float>(i, 2 * i);
        shv[i] = std::complex<float>(i + 0.1, i + 0.3);
    }

    slcHH.setBlock(shh, 0, 0, width, length);
    slcHV.setBlock(shv, 0, 0, width, length);
}
