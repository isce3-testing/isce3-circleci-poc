//

#include <gtest/gtest.h>

#include "gdal_alg.h"
#include "isce3/io/IH5Dataset.h"
#include "isce3/io/Raster.h"

template<class A, class B>
struct TypeDefs {
    typedef A firstType;
    typedef B secondType;
};

template<class T>
struct IH5Test : public ::testing::Test {
protected:
    IH5Test() {}

    void SetUp()
    {
        GDALAllRegister();
        isce3::io::GDALRegister_IH5();
    }

    void TearDown() { GDALDestroyDriverManager(); }
};

// Types for which to test
typedef ::testing::Types<TypeDefs<unsigned char, short int>,
        TypeDefs<unsigned char, unsigned short int>,
        TypeDefs<unsigned char, int>, TypeDefs<unsigned char, unsigned int>,
        TypeDefs<unsigned char, float>, TypeDefs<unsigned char, double>,
        TypeDefs<unsigned char, std::complex<float>>,
        TypeDefs<unsigned char, std::complex<double>>,
        TypeDefs<short int, unsigned short int>, TypeDefs<short int, int>,
        TypeDefs<short int, unsigned int>, TypeDefs<short int, float>,
        TypeDefs<short int, double>, TypeDefs<short int, std::complex<float>>,
        TypeDefs<short int, std::complex<double>>,
        TypeDefs<unsigned short int, int>,
        TypeDefs<unsigned short int, unsigned int>,
        TypeDefs<unsigned short int, float>,
        TypeDefs<unsigned short int, double>,
        TypeDefs<unsigned short int, std::complex<float>>,
        TypeDefs<unsigned short int, std::complex<double>>,
        TypeDefs<int, unsigned int>, TypeDefs<int, float>,
        TypeDefs<int, double>, TypeDefs<int, std::complex<float>>,
        TypeDefs<int, std::complex<double>>, TypeDefs<unsigned int, float>,
        TypeDefs<unsigned int, double>,
        TypeDefs<unsigned int, std::complex<float>>,
        TypeDefs<unsigned int, std::complex<double>>, TypeDefs<float, double>,
        TypeDefs<float, std::complex<float>>,
        TypeDefs<float, std::complex<double>>,
        TypeDefs<double, std::complex<double>>>
        MyTypes;

// Setup test suite
TYPED_TEST_SUITE(IH5Test, MyTypes);

// This is a typed test
TYPED_TEST(IH5Test, nochunk)
{

    // Typedefs for individual types
    typedef typename TypeParam::firstType FirstParam;
    typedef typename TypeParam::secondType SecondParam;

    // Create a matrix of typeparam
    int width = 20;
    int length = 30;
    isce3::core::Matrix<FirstParam> _inmatrix(length, width);
    isce3::core::Matrix<SecondParam> _outmatrix(length, width);
    for (size_t ii = 0; ii < (width * length); ii++) {
        _inmatrix(ii) = (ii % 255);
        _outmatrix(ii) = (ii % 255);
    }

    // Get checksum for the data
    int matsum;
    {
        isce3::io::Raster matRaster(_outmatrix);
        ASSERT_EQ(matRaster.dtype(1), isce3::io::asGDT<SecondParam>);
        matsum = GDALChecksumImage(
                matRaster.dataset()->GetRasterBand(1), 0, 0, width, length);
    }

    // Create a HDF5 file
    std::string wfilename = "castwrite_" +
                            std::string(typeid(FirstParam).name()) +
                            std::string(typeid(SecondParam).name()) + ".h5";
    struct stat buffer;
    if (stat(wfilename.c_str(), &buffer) == 0)
        std::remove(wfilename.c_str());
    isce3::io::IH5File fic(wfilename, 'x');

    isce3::io::IGroup grp = fic.openGroup("/");
    std::array<int, 2> shp = {length, width};
    isce3::io::IDataSet dset =
            grp.createDataSet<SecondParam>(std::string("data"), shp);
    {
        isce3::io::Raster img(dset.toGDAL(), GA_Update);
        img.setBlock(_inmatrix, 0, 0, 1);

        // Check contents of the HDF5 file
        ASSERT_EQ(img.width(), width);
        ASSERT_EQ(img.length(), length);
        ASSERT_EQ(img.dtype(1), isce3::io::asGDT<SecondParam>);

        int hsum = GDALChecksumImage(
                img.dataset()->GetRasterBand(1), 0, 0, width, length);
        ASSERT_EQ(hsum, matsum);

        SecondParam val;
        img.getValue(val, 11, 13, 1);
        ASSERT_EQ(val, _outmatrix(13, 11));
    }
    // Cleanup
    dset.close();
    grp.close();
    fic.close();
    if (stat(wfilename.c_str(), &buffer) == 0)
        std::remove(wfilename.c_str());
}

// This is a typed test
TYPED_TEST(IH5Test, chunk)
{

    // Typedefs for individual types
    typedef typename TypeParam::firstType FirstParam;
    typedef typename TypeParam::secondType SecondParam;

    // Create a matrix of typeparam
    int width = 250;
    int length = 200;
    isce3::core::Matrix<FirstParam> _inmatrix(length, width);
    isce3::core::Matrix<SecondParam> _outmatrix(length, width);
    for (size_t ii = 0; ii < (width * length); ii++) {
        _inmatrix(ii) = (ii % 255);
        _outmatrix(ii) = (ii % 255);
    }

    // Get checksum for the data
    int matsum;
    {
        isce3::io::Raster matRaster(_outmatrix);
        ASSERT_EQ(matRaster.dtype(1), isce3::io::asGDT<SecondParam>);
        matsum = GDALChecksumImage(
                matRaster.dataset()->GetRasterBand(1), 0, 0, width, length);
    }

    // Create a HDF5 file
    std::string wfilename = "castwrite_" +
                            std::string(typeid(FirstParam).name()) +
                            std::string(typeid(SecondParam).name()) + ".h5";
    struct stat buffer;
    if (stat(wfilename.c_str(), &buffer) == 0)
        std::remove(wfilename.c_str());
    isce3::io::IH5File fic(wfilename, 'x');

    isce3::io::IGroup grp = fic.openGroup("/");
    std::array<int, 2> shp = {length, width};
    isce3::io::IDataSet dset =
            grp.createDataSet<SecondParam>(std::string("data"), shp, 1);
    {
        isce3::io::Raster img(dset.toGDAL(), GA_Update);
        img.setBlock(_inmatrix, 0, 0, 1);

        // Check contents of the HDF5 file
        ASSERT_EQ(img.width(), width);
        ASSERT_EQ(img.length(), length);
        ASSERT_EQ(img.dtype(1), isce3::io::asGDT<SecondParam>);

        // Read data type with casting into another matrix
        // And compute check sum
        int hsum = GDALChecksumImage(
                img.dataset()->GetRasterBand(1), 0, 0, width, length);
        ASSERT_EQ(hsum, matsum);

        SecondParam val;
        // Quad 1
        img.getValue(val, 2, 3, 1);
        ASSERT_EQ(val, _outmatrix(3, 2));

        // Quad 2
        img.getValue(val, 130, 5, 1);
        ASSERT_EQ(val, _outmatrix(5, 130));

        // Quad 3
        img.getValue(val, 6, 129, 1);
        ASSERT_EQ(val, _outmatrix(129, 6));

        // Quad 4
        img.getValue(val, 128, 135, 1);
        ASSERT_EQ(val, _outmatrix(135, 128));
    }

    // Cleanup
    dset.close();
    grp.close();
    fic.close();
    if (stat(wfilename.c_str(), &buffer) == 0)
        std::remove(wfilename.c_str());
}

// Main
int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// end of file
