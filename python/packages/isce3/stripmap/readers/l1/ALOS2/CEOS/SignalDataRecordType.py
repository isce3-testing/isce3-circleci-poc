###SAR Signal Data Record
def SignalDataRecordType(pixels=None, bytesperpixel=None):
    """
    SAR Signal Data Record.
    http://www.ga.gov.au/__data/assets/pdf_file/0019/11719/GA10287.pdf Pg:3-51.
    """
    from isce3.parsers.CEOS.BasicTypes import (BinaryType, BlankType,
                                               IntegerType, MultiType,
                                               StringType)
    from isce3.parsers.CEOS.CEOSHeaderType import CEOSHeaderType

    return MultiType(
        CEOSHeaderType().mapping
        + [
            ("SARImageDataLineNumber", BinaryType(">i4")),
            ("SARImageDataRecordIndex", BinaryType(">i4")),
            ("ActualCountOfLeftFillPixels", BinaryType(">i4")),
            ("ActualCountOfDataPixels", BinaryType(">i4")),
            ("ActualCountOfRightFillPixels", BinaryType(">i4")),
            ("SensorParametersUpdateFlag", BinaryType(">i4")),
            ("SensorAcquisitionYear", BinaryType(">i4")),
            ("SensorAcquisitionDayOfYear", BinaryType(">i4")),
            ("SensorAcquisitionmsecsOfDay", BinaryType(">i4")),
            ("SARChannelIndicator", BinaryType(">i2")),
            ("SARChannelCode", BinaryType(">i2")),
            ("TransmitPolarization", BinaryType(">i2")),
            ("ReceivePolarization", BinaryType(">i2")),
            ("PRFInmHz", BinaryType(">i4")),
            ("ScanIDForScanSAR", BinaryType(">i4")),
            ("OnboardRangeCompressedFlag", BinaryType(">i2")),
            ("PulseTypeDesignator", BinaryType(">i2")),
            ("ChirpLengthInns", BinaryType(">i4")),
            ("ChirpConstantCoefficientInHz", BinaryType(">i4")),
            ("ChirpLinearCoefficientInHzperusec", BinaryType(">i4")),
            ("ChirpQuadraticCoefficient", BinaryType(">i4")),
            ("SensorAcquisitionusecsOfDay", BinaryType(">Q")),
            ("ReceiverGainIndB", BinaryType(">i4")),
            ("NoughtLineFlag", BinaryType(">i4")),
            ("ElectronicAntennaElevationAngleinudegrees", BinaryType(">i4")),
            ("MechanicalAntennaElevationAngleinudegrees", BinaryType(">i4")),
            ("ElectronicAntennaSquintAngleinudegrees", BinaryType(">i4")),
            ("MechanicalAntennaSquintAngleinudegrees", BinaryType(">i4")),
            ("SlantRangeToFirstSampleInm", BinaryType(">i4")),
            ("DataRecordWindowPositionInns", BinaryType(">i4")),
            ("blanks2", BlankType(4)),
            ("PlatformPositionParametersUpdateFlag", BinaryType(">i4")),
            ("PlatformLatitudeInudegrees", BinaryType(">i4")),
            ("PlatformLongitudeInudegrees", BinaryType(">i4")),
            ("PlatformAltitudeInm", BinaryType(">i4")),
            ("PlatformGroundSpeedIncmpersec", BinaryType(">i4")),
            ("PlatformVelocityIncmpersec", BinaryType(">i4", count=3)),
            ("PlatformAcceleration", BinaryType(">i4", count=3)),
            ("PlatformTrackAngleinudegrees", BinaryType(">i4")),
            ("PlatformTrackAngle2inudegrees", BinaryType(">i4")),
            ("PlatformPitchAngleinudegrees", BinaryType(">i4")),
            ("PlatformRollAngleinudegrees", BinaryType(">i4")),
            ("PlatformYawAngleinudegrees", BinaryType(">i4")),
            ("blanks3", BlankType(92)),
            ("PALSARFrameCounter", BinaryType(">i4")),
            ("PALSARAuxData", BlankType(256)),
            (
                "SARRawSignalData",
                BinaryType(">f{}".format(bytesperpixel), count=pixels),
            ),
        ]
    )
