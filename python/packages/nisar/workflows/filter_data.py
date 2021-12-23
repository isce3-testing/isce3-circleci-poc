import os
from dataclasses import dataclass

import h5py
import isce3
import numpy as np
from osgeo import gdal


@dataclass
class BlockParam:
    """
    Class for block specific parameters
    Facilitate block parameters exchange between functions
    """

    # Length of current block to filter; padding not included
    block_length: int

    # First line to write to for current block
    write_start_line: int

    # First line to read from dataset for current block
    read_start_line: int

    # Number of lines to read from dataset for current block
    read_length: int

    # Padding to be applied to read in current block. First tuple is padding to
    # be applied to top/bottom (along length). Second tuple is padding to be
    # applied to left/right (along width). Values in second tuple do not change;
    # included in class so one less value is passed between functions.
    block_pad: tuple

    # Width of current block. Value does not change per block; included to
    # in class so one less value is to be passed between functions.
    data_width: int


def np2gdal_dtype(np_dtype):
    dict_np2gdal = {
        np.byte: gdal.GDT_Byte,
        np.ushort: gdal.GDT_UInt16,
        np.short: gdal.GDT_Int16,
        np.uintc: gdal.GDT_UInt32,
        np.intc: gdal.GDT_Int32,
        np.float32: gdal.GDT_Float32,
        np.float64: gdal.GDT_Float64,
        np.complex64: gdal.GDT_CFloat32,
        np.complex128: gdal.GDT_CFloat64,
    }
    if np_dtype not in dict_np2gdal:
        # throw unsupported error
        pass
    else:
        return dict_np2gdal[int_dtype]


def block_param_generator(lines_per_block, data_shape, pad_shape):
    """Generator for block specific parameter class.

    Parameters
    ----------
    lines_per_block: int
        Lines to be processed per block (in batch).
    data_shape: tuple(int, int)
        Length and width of input raster.
    pad_shape: tuple(int, int)
        Padding for the length and width of block to be filtered.

    Returns
    -------
    _: BlockParam
        BlockParam object for current block
    """
    data_length, data_width = data_shape
    pad_length, pad_width = pad_shape

    # Calculate number of blocks to break raster into
    num_blocks = int(np.ceil(data_length / lines_per_block))

    for block in range(num_blocks):
        start_line = block * lines_per_block

        # Discriminate between first, last, and middle blocks
        first_block = block == 0
        last_block = block == num_blocks - 1 or num_blocks == 1
        middle_block = not first_block and not last_block

        # Determine block size; Last block uses leftover lines
        block_length = data_length - start_line if last_block else lines_per_block

        # Determine padding along length. Full padding for middle blocks
        # Half padding for start and end blocks
        read_length_pad = pad_length if middle_block else pad_length // 2

        # Determine 1st line of output
        write_start_line = block * lines_per_block

        # Determine 1st dataset line to read. Subtract half padding length
        # to account for additional lines to be read.
        read_start_line = block * lines_per_block - pad_length // 2

        # If applicable, save negative start line as deficit to account for later
        read_start_line, start_line_deficit = (
            (0, read_start_line) if read_start_line < 0 else (read_start_line, 0)
        )

        # Initial guess at number lines to read; accounting for negative start at the end
        read_length = block_length + read_length_pad
        if not first_block:
            read_length -= abs(start_line_deficit)

        # Check for over-reading and adjust lines read as needed
        end_line_deficit = min(data_length - read_start_line - read_length, 0)
        read_length -= abs(end_line_deficit)

        # Determine block padding in length
        if first_block:
            # Only the top part of the block should be padded. If end_deficit_line=0
            # we have a sufficient number of lines to be read in the subsequent block
            top_pad = pad_length // 2
            bottom_pad = abs(end_line_deficit)
        elif last_block:
            # Only the bottom part of the block should be padded
            top_pad = abs(start_line_deficit) if start_line_deficit < 0 else 0
            bottom_pad = pad_length // 2
        else:
            # Top and bottom should be added taking into account line deficit
            top_pad = abs(start_line_deficit) if start_line_deficit < 0 else 0
            bottom_pad = abs(end_line_deficit)

        block_pad = ((top_pad, bottom_pad), (pad_width // 2, pad_width // 2))

        yield BlockParam(
            block_length,
            write_start_line,
            read_start_line,
            read_length,
            block_pad,
            data_width,
        )

    return


def get_raster_info(raster):
    """Determine raster shape based on raster
        type (h5py.Dataset or GDAL-friendly raster).

    Parameters
    ----------
    raster: h5py.Dataset or str
        Raster whose size is to be determined. String value represents
        filepath for GDAL rasters.

    Returns
    -------
        data_width: int
            Width of raster.
        data_length: int
            Length of raster.
    """
    if isinstance(raster, h5py.Dataset):
        return raster.shape, raster.dtype
    else:
        # Open input data using GDAL to get raster length
        ds = gdal.Open(raster, gdal.GA_ReadOnly)
        data_length = ds.RasterYSize
        data_width = ds.RasterXSize
        data_type = ds.GetRasterBand(1).DataType
        return (data_length, data_width), data_type


def get_raster_block(raster, block_param):
    """Get a block of data from raster.
        Raster can be a HDF5 file or a GDAL-friendly raster

    Parameters
    ----------
    raster: h5py.Dataset or str
        Raster where a block is to be read from. String value represents a
        filepath for GDAL rasters.
    block_param: BlockParam
        Object specifying size of block and where to read from raster,
        and amount of padding for the read array

    Returns
    -------
    data_block: np.ndarray
        Block read from raster with shape specified in block_param.
    """
    if isinstance(raster, h5py.Dataset):
        data_block = np.empty(
            (block_param.read_length, block_param.data_width), dtype=raster.dtype
        )
        raster.read_direct(
            data_block,
            np.s_[
                block_param.read_start_line : block_param.read_start_line
                + block_param.read_length,
                :,
            ],
        )
    else:
        # Open input data using GDAL to get raster length
        ds_data = gdal.Open(raster, gdal.GA_Update)
        data_block = ds_data.GetRasterBand(1).ReadAsArray(
            0,
            block_param.read_start_line,
            block_param.data_width,
            block_param.read_length,
        )

    # Pad igram_block with zeros according to pad_length/pad_width
    data_block = np.pad(
        data_block, block_param.block_pad, mode="constant", constant_values=0
    )

    return data_block


def write_raster_block(out_raster, data, block_param):
    """Write processed block to out_raster.

    Parameters
    ----------
    out_raster: h5py.Dataset or str
        Raster where data (i.e., filtered data) needs to be written.
        String value represents filepath for GDAL rasters.
    data: np.ndarray
        Filtered data to write to out_raster.
    block_param: BlockParam
        Object specifying where and how much to write to out_raster.
    """
    if isinstance(out_raster, h5py.Dataset):
        out_raster.write_direct(
            data,
            dest_sel=np.s_[
                block_param.write_start_line : block_param.write_start_line
                + block_param.block_length,
                :,
            ],
        )
    else:
        ds_data = gdal.Open(out_raster, gdal.GA_Update)
        ds_data.GetRasterBand(1).WriteArray(
            data, xoff=0, yoff=block_param.write_start_line
        )


def filter_data(
    input_data,
    lines_per_block,
    kernel_rows,
    kernel_cols,
    output_data=None,
    mask_path=None,
):
    """Filter data using two separable 1D kernels.

    Parameters
    ----------
    input_data: str
        File path to input data raster (GDAL-friendly)
    lines_per_block: int
        Number of lines to process in batch
    kernel_rows: float array
        1D kernel along rows direction
    kernel_cols: float array
        1D kernel along columns direction
    output_data: h5py.Dataset or str
        Raster where a block needs to be written to. String value represents
        file path for GDAL rasters. If not provided, input_data is overwritten
        with the output filtered data
    mask_path: str
        Filepath to the mask to use during filtering

    Returns
    -------
    """

    data_shape, data_type = get_raster_info(input_data)
    data_length, data_width = data_shape

    # Determine the amount of padding
    pad_length = 2 * (len(kernel_rows) // 2)
    pad_width = 2 * (kernel_cols.shape[1] // 2)
    pad_shape = (pad_length, pad_width)

    # Determine number of blocks to process
    lines_per_block = min(data_length, lines_per_block)

    # Start block processing
    block_params = block_param_generator(lines_per_block, data_shape, pad_shape)
    for block_param in block_params:
        # Read a block of data. If hdf5_dset is set, read a block of data
        # directly from the hdf5 file. Otherwise, use gdal to read block of data
        data_block = get_raster_block(input_data, block_param)

        # Get if filtering needs to be performed with or without a mask
        if mask_path is not None:
            # Use gdal to extract a mask block, pad the mask (mask need to be same shape as input)
            ds_mask = gdal.Open(mask_path, gdal.GA_ReadOnly)
            mask_block = ds_mask.GetRasterBand(1).ReadAsArray(
                0,
                block_param.read_start_line,
                block_param.data_width,
                block_param.read_length,
            )
            mask_block = np.pad(
                mask_block, block_param.block_pad, mode="constant", constant_values=0
            )
            filt_data_block = isce3.signal.convolve2D(
                data_block, mask_block, kernel_cols, kernel_rows, False
            )
        else:
            filt_data_block = isce3.signal.convolve2D(
                data_block, kernel_cols, kernel_rows, False
            )
        # If no value provided for output_data, then overwrite existing
        # input with filtered output
        # Otherwise write filtered output to output_data
        out_raster = input_data if output_data is None else output_data

        # If writing to GDAL raster, prepare file
        if not isinstance(out_raster, h5py.Dataset) and not os.path.isfile(out_raster):
            raster = isce3.io.Raster(
                path=out_raster,
                width=data_width,
                length=data_length,
                num_bands=1,
                dtype=data_type,
                driver_name="GTiff",
            )
            del raster

        write_raster_block(out_raster, filt_data_block, block_param)
