//

#include <gtest/gtest.h>

#include <isce3/io/IH5Dataset.h>
#include <isce3/io/Raster.h>

#include "gdal_alg.h"

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
typedef ::testing::Types<unsigned char, short int, unsigned short int, int,
        unsigned int, float, double, std::complex<float>, std::complex<double>>
        MyTypes;

// Setup test suite
TYPED_TEST_SUITE(IH5Test, MyTypes);

// This is a typed test
TYPED_TEST(IH5Test, nochunk)
{
    // Create a matrix of typeparam
    int width = 20;
    int length = 30;
    isce3::core::Matrix<TypeParam> _matrix(length, width);
    for (size_t ii = 0; ii < (width * length); ii++)
        _matrix(ii) = (ii % 255);

    // Get checksum for the data
    isce3::io::Raster matRaster(_matrix);
    int matsum = GDALChecksumImage(
            matRaster.dataset()->GetRasterBand(1), 0, 0, width, length);

    // Create a HDF5 file
    std::string wfilename =
            "nativewrite_" + std::string(typeid(TypeParam).name()) + ".h5";
    struct stat buffer;
    if (stat(wfilename.c_str(), &buffer) == 0)
        std::remove(wfilename.c_str());
    isce3::io::IH5File fic(wfilename, 'x');

    isce3::io::IGroup grp = fic.openGroup("/");
    std::array<int, 2> shp = {length, width};
    isce3::io::IDataSet dset =
            grp.createDataSet<TypeParam>(std::string("data"), shp);
    {
        isce3::io::Raster img(dset.toGDAL(), GA_Update);
        img.setBlock(_matrix, 0, 0, 1);

        // Check contents of the HDF5 file
        ASSERT_EQ(img.width(), width);
        ASSERT_EQ(img.length(), length);
        ASSERT_EQ(img.dtype(1), isce3::io::asGDT<TypeParam>);

        int hsum = GDALChecksumImage(
                img.dataset()->GetRasterBand(1), 0, 0, width, length);
        ASSERT_EQ(hsum, matsum);

        TypeParam val;
        img.getValue(val, 11, 13, 1);

        ASSERT_EQ(val, _matrix(13, 11));
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
    // Create a matrix of typeparam
    int width = 250;
    int length = 200;
    isce3::core::Matrix<TypeParam> _matrix(length, width);
    for (size_t ii = 0; ii < (width * length); ii++)
        _matrix(ii) = (ii % 255);

    // Get checksum for the data
    isce3::io::Raster matRaster(_matrix);
    int matsum = GDALChecksumImage(
            matRaster.dataset()->GetRasterBand(1), 120, 120, 10, 10);

    // Create a HDF5 file
    std::string wfilename =
            "nativewrite_" + std::string(typeid(TypeParam).name()) + ".h5";
    struct stat buffer;
    if (stat(wfilename.c_str(), &buffer) == 0)
        std::remove(wfilename.c_str());
    isce3::io::IH5File fic(wfilename, 'x');

    isce3::io::IGroup grp = fic.openGroup("/");
    std::array<int, 2> shp = {length, width};
    isce3::io::IDataSet dset =
            grp.createDataSet<TypeParam>(std::string("data"), shp, 1);
    {
        isce3::io::Raster img(dset.toGDAL(), GA_Update);
        img.setBlock(_matrix, 0, 0, 1);

        // Check contents of the HDF5 file
        ASSERT_EQ(img.width(), width);
        ASSERT_EQ(img.length(), length);
        ASSERT_EQ(img.dtype(1), isce3::io::asGDT<TypeParam>);

        int hsum = GDALChecksumImage(
                img.dataset()->GetRasterBand(1), 120, 120, 10, 10);
        ASSERT_EQ(hsum, matsum);

        TypeParam val;
        // Quad 1
        img.getValue(val, 2, 3, 1);
        ASSERT_EQ(val, _matrix(3, 2));

        // Quad 2
        img.getValue(val, 130, 5, 1);
        ASSERT_EQ(val, _matrix(5, 130));

        // Quad 3
        img.getValue(val, 6, 129, 1);
        ASSERT_EQ(val, _matrix(129, 6));

        // Quad 4
        img.getValue(val, 128, 135, 1);
        ASSERT_EQ(val, _matrix(135, 128));
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
