#!/usr/bin/env python3

import isce3
import iscetest
import pytest
from nisar.products.readers import SLC, open_product
from nisar.products.readers.Raw import Raw


def test_radargridparameters():
    # Test creation of an L0B RRSD product
    nisar_product_obj = open_product(iscetest.data + "REE_L0B_out17.h5")
    assert isinstance(nisar_product_obj, Raw)
    assert nisar_product_obj.productType == "RRSD"

    # Test creation of an L1 RSLC product
    nisar_product_obj = open_product(iscetest.data + "envisat.h5")
    assert isinstance(nisar_product_obj, SLC)
    assert nisar_product_obj.productType == "RSLC"

    # Test creation of an L1 RSLC product
    nisar_product_obj = open_product(iscetest.data + "Greenland.h5")
    assert isinstance(nisar_product_obj, SLC)
    assert nisar_product_obj.productType == "RSLC"

    # Test creation of an L1 RSLC product
    nisar_product_obj = open_product(iscetest.data + "winnipeg.h5")
    assert isinstance(nisar_product_obj, SLC)
    assert nisar_product_obj.productType == "RSLC"
