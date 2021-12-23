//-*- C++ -*-
//-*- coding: utf-8 -*-
//
// Author: Bryan V. Riel
// Copyright 2018
//

#include <fstream>
#include <iostream>

#include <gtest/gtest.h>

// isce3::core
#include <isce3/core/Orbit.h>
#include <isce3/core/Serialization.h>
#include <isce3/io/IH5.h>

TEST(OrbitTest, CheckArchive)
{
    // Make an orbit
    isce3::core::Orbit orbit;

    // Open the HDF5 product
    std::string h5file(TESTDATA_DIR "envisat.h5");
    isce3::io::IH5File file(h5file);

    // Open group containing orbit
    isce3::io::IGroup group =
            file.openGroup("/science/LSAR/SLC/metadata/orbit");

    // Deserialize the orbit
    isce3::core::loadFromH5(group, orbit);

    // Check we have the right number of state vectors
    ASSERT_EQ(orbit.size(), 11);

    // Check the position of middle vector
    ASSERT_NEAR(orbit.position(5)[0], -2305250.945, 1.0e-6);
    ASSERT_NEAR(orbit.position(5)[1], -5443208.984, 1.0e-6);
    ASSERT_NEAR(orbit.position(5)[2], 4039406.416, 1.0e-6);

    // Check the velocity of middle vector
    ASSERT_NEAR(orbit.velocity(5)[0], -3252.930393, 1.0e-6);
    ASSERT_NEAR(orbit.velocity(5)[1], -3129.103767, 1.0e-6);
    ASSERT_NEAR(orbit.velocity(5)[2], -6055.488170, 1.0e-6);

    // Check date of middle vector
    isce3::core::DateTime dtime = orbit.referenceEpoch() + orbit.time(5);
    ASSERT_EQ(dtime.isoformat(), "2003-02-26T17:55:28.000000000");
}

TEST(OrbitTest, CheckWrite)
{
    // Make an orbit
    isce3::core::Orbit orbit;

    // Load orbit data
    {
        // Open the HDF5 product
        std::string h5file(TESTDATA_DIR "envisat.h5");
        isce3::io::IH5File file(h5file);

        // Open group containing orbit
        isce3::io::IGroup group =
                file.openGroup("/science/LSAR/SLC/metadata/orbit");

        // Deserialize the orbit
        isce3::core::loadFromH5(group, orbit);
        orbit.interpMethod(isce3::core::OrbitInterpMethod::Legendre);
    }

    // Write orbit data
    {
        // Create a dummy hdf5 file
        std::string dummyfile("dummy_orbit.h5");
        isce3::io::IH5File dummy(dummyfile, 'x');

        // Write orbit to dataset
        isce3::io::IGroup group = dummy.createGroup("orbit");
        isce3::core::saveToH5(group, orbit);
    }

    // Load a new orbit from created file
    isce3::core::Orbit newOrb;
    std::string h5file("dummy_orbit.h5");
    isce3::io::IH5File file(h5file);
    isce3::io::IGroup group = file.openGroup("orbit");
    isce3::core::loadFromH5(group, newOrb);

    // Check equality
    ASSERT_EQ(orbit, newOrb);
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// end of file
