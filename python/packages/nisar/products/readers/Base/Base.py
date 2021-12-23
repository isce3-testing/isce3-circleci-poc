# -*- coding: utf-8 -*-

import os

import h5py
import isce3
import journal
import pyre

from ..protocols import ProductReader


def get_hdf5_file_root_path(filename: str, root_path: str = None) -> str:
    """
    Return root path from HDF5 file. If a root path is provided as
    a parameter and it exists, it will have precedence over the
    root path from the HDF5 file

    Parameters
    ----------
    filename : str
        HDF5 filename
    root_path : str
        Preliminary root path to check (e.g., XSAR, PSAR) before default root
        path list

    Returns
    -------
    str
        Root path from HDF5 file

    """

    error_channel = journal.error("get_hdf5_file_root_path")

    SCIENCE_PATH = "/science/"
    NISAR_FREQ_BAND_LIST = ["SSAR", "LSAR"]
    with h5py.File(filename, "r", libver="latest", swmr=True) as f:
        if root_path is not None and root_path in f:
            return root_path
        science_group = f[SCIENCE_PATH]
        for freq_band in NISAR_FREQ_BAND_LIST:
            if freq_band not in science_group:
                continue
            return SCIENCE_PATH + freq_band

    error_msg = (
        "HDF5 could not find NISAR frequency"
        f" band group LSAR or SSAR in file: {filename}"
    )

    error_channel.log(error_msg)


class Base(pyre.component, family="nisar.productreader.base", implements=ProductReader):
    """
    Base class for NISAR products.

    Contains common functionality that gets reused across products.
    """

    _CFPath = pyre.properties.str(default="/")
    _CFPath.doc = "Absolute path to scan for CF convention metadata"

    _RootPath = pyre.properties.str(default=None)
    _RootPath.doc = "Absolute path to SAR data from L-SAR/S-SAR"

    _IdentificationPath = pyre.properties.str(default="identification")
    _IdentificationPath.doc = (
        "Absolute path ath to unique product identification information"
    )

    _ProductType = pyre.properties.str(default=None)
    _ProductType.doc = "The type of the product."

    _MetadataPath = pyre.properties.str(default="metadata")
    _MetadataPath.doc = "Relative path to metadata associated with standard product"

    _ProcessingInformation = pyre.properties.str(default="processingInformation")
    _ProcessingInformation.doc = (
        "Relative path to processing information associated with the product"
    )

    _SwathPath = pyre.properties.str(default="swaths")
    _SwathPath.doc = "Relative path to swaths associated with standard product"

    _GridPath = pyre.properties.str(default="grids")
    _GridPath.doc = "Relative path to grids associated with standard product"

    productValidationType = pyre.properties.str(default="BASE")
    productValidationType.doc = "Validation tag to compare identification information against to ensure that the right product type is being used."

    def __init__(self, hdf5file=None, **kwds):
        """
        Constructor.
        """

        # Set error channel
        self.error_channel = journal.error("Base")

        # Check hdf5file
        if hdf5file is None:
            err_str = f"Please provide an input HDF5 file"
            self.error_channel.log(err_str)

        # Filename
        self.filename = hdf5file

        # Identification information
        self.identification = None

        # Polarization dictionary
        self.polarizations = {}

        self.populateIdentification()

        self.identification.productType = self._ProductType

        if self._ProductType is None:
            return

        self.parsePolarizations()

    @pyre.export
    def getSwathMetadata(self, frequency="A"):
        """
        Returns metadata corresponding to given frequency.
        """
        return isce3.product.Swath(self.filename, frequency)

    @pyre.export
    def getRadarGrid(self, frequency="A"):
        """
        Return radarGridParameters object
        """
        return isce3.product.RadarGridParameters(self.filename, frequency)

    @pyre.export
    def getGridMetadata(self, frequency="A"):
        """
        Returns metadata corresponding to given frequency.
        """
        raise NotImplementedError

    @pyre.export
    def getOrbit(self):
        """
        extracts orbit
        """
        with h5py.File(self.filename, "r", libver="latest", swmr=True) as fid:
            orbitPath = os.path.join(self.MetadataPath, "orbit")
            return isce3.core.Orbit.load_from_h5(fid[orbitPath])

    @pyre.export
    def getDopplerCentroid(self, frequency="A"):
        """
        Extract the Doppler centroid
        """
        dopplerPath = os.path.join(
            self.ProcessingInformationPath,
            "parameters",
            "frequency" + frequency,
            "dopplerCentroid",
        )

        zeroDopplerTimePath = os.path.join(
            self.ProcessingInformationPath, "parameters/zeroDopplerTime"
        )

        slantRangePath = os.path.join(
            self.ProcessingInformationPath, "parameters/slantRange"
        )
        # extract the native Doppler dataset
        with h5py.File(self.filename, "r", libver="latest", swmr=True) as fid:
            doppler = fid[dopplerPath][:]
            zeroDopplerTime = fid[zeroDopplerTimePath][:]
            slantRange = fid[slantRangePath][:]

        dopplerCentroid = isce3.core.LUT2d(
            xcoord=slantRange, ycoord=zeroDopplerTime, data=doppler
        )
        return dopplerCentroid

    @pyre.export
    def getZeroDopplerTime(self):
        """
        Extract the azimuth time of the zero Doppler grid
        """

        zeroDopplerTimePath = os.path.join(self.SwathPath, "zeroDopplerTime")
        with h5py.File(self.filename, "r", libver="latest", swmr=True) as fid:
            zeroDopplerTime = fid[zeroDopplerTimePath][:]

        return zeroDopplerTime

    @pyre.export
    def getSlantRange(self, frequency="A"):
        """
        Extract the slant range of the zero Doppler grid
        """

        slantRangePath = os.path.join(
            self.SwathPath, "frequency" + frequency, "slantRange"
        )

        with h5py.File(self.filename, "r", libver="latest", swmr=True) as fid:
            slantRange = fid[slantRangePath][:]

        return slantRange

    def parsePolarizations(self):
        """
        Parse HDF5 and identify polarization channels available for each frequency.
        """
        from nisar.h5 import bytestring, extractWithIterator

        try:
            frequencyList = self.frequencies
        except:
            raise RuntimeError(
                "Cannot determine list of available frequencies without parsing Product Identification"
            )

        ###Determine if product has swaths / grids
        if self.productType.startswith("G"):
            folder = self.GridPath
        else:
            folder = self.SwathPath

        with h5py.File(self.filename, "r", libver="latest", swmr=True) as fid:
            for freq in frequencyList:
                root = os.path.join(folder, "frequency{0}".format(freq))
                polList = extractWithIterator(
                    fid[root],
                    "listOfPolarizations",
                    bytestring,
                    msg="Could not determine polarization for frequency{0}".format(
                        freq
                    ),
                )
                self.polarizations[freq] = [p.upper() for p in polList]

        return

    @property
    def CFPath(self):
        return self._CFPath

    @property
    def RootPath(self):
        if self._RootPath is None:
            self._RootPath = get_hdf5_file_root_path(self.filename)
        return self._RootPath

    @property
    def IdentificationPath(self):
        return os.path.join(self.RootPath, self._IdentificationPath)

    @property
    def ProductPath(self):
        return os.path.join(self.RootPath, self.productType)

    @property
    def MetadataPath(self):
        return os.path.join(self.ProductPath, self._MetadataPath)

    @property
    def ProcessingInformationPath(self):
        return os.path.join(self.MetadataPath, self._ProcessingInformation)

    @property
    def SwathPath(self):
        return os.path.join(self.ProductPath, self._SwathPath)

    @property
    def GridPath(self):
        return os.path.join(self.ProductPath, self._GridPath)

    @property
    def productType(self):
        return self.identification.productType

    def populateIdentification(self):
        """
        Read in the Identification information and assert identity.
        """
        from .Identification import Identification

        with h5py.File(self.filename, "r", libver="latest", swmr=True) as fileID:
            h5grp = fileID[self.IdentificationPath]
            self.identification = Identification(h5grp)

    @property
    def frequencies(self):
        """
        Return list of frequencies in the product.
        """
        return self.identification.listOfFrequencies

    @staticmethod
    def validate(self, hdf5file):
        """
        Validate a given HDF5 file.
        """
        raise NotImplementedError

    def computeBoundingBox(self, epsg=4326):
        """
        Compute the bounding box as a polygon in given projection system.
        """
        raise NotImplementedError


# end of file
