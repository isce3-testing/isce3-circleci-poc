# /usr/bin/env python3

import json
from pathlib import Path

import h5py
import numpy as np
import pybind_isce3 as isce
from iscetest import data as test_data_dir
from nisar.workflows.point_target_info import analyze_point_target, tofloatvals

from ...focus.backproject import load_h5

c = isce.core.speed_of_light


def test_backproject():
    # load point target simulation data
    filename = Path(test_data_dir) / "point-target-sim-rc.h5"
    d = load_h5(filename)

    # eww gross
    signal_data = d["signal_data"]
    radar_grid = d["radar_grid"]
    orbit = d["orbit"]
    doppler = d["doppler"]
    center_frequency = d["center_frequency"]
    range_sampling_rate = d["range_sampling_rate"]
    dem = d["dem"]
    dry_tropo_model = d["dry_tropo_model"]
    target_azimuth = d["target_azimuth"]
    target_range = d["target_range"]

    # range bandwidth (Hz)
    B = 20e6

    # desired azimuth resolution (m)
    azimuth_res = 6.0

    # output chip size
    nchip = 129

    # how much to upsample the output for point target analysis
    upsample_factor = 128

    # create 9-point Knab kernel
    # use tabulated kernel for performance
    kernel = isce.core.KnabKernel(9.0, B / range_sampling_rate)
    kernel = isce.core.TabulatedKernelF32(kernel, 2048)

    # create output radar grid centered on the target
    dt = radar_grid.az_time_interval
    dr = radar_grid.range_pixel_spacing
    t0 = target_azimuth - 0.5 * (nchip - 1) * dt
    r0 = target_range - 0.5 * (nchip - 1) * dr
    out_grid = isce.product.RadarGridParameters(
        t0,
        radar_grid.wavelength,
        radar_grid.prf,
        r0,
        dr,
        radar_grid.lookside,
        nchip,
        nchip,
        orbit.reference_epoch,
    )

    # init output buffer
    out = np.empty((nchip, nchip), np.complex64)

    # collect input & output radar_grid, orbit, and Doppler
    in_geometry = isce.container.RadarGeometry(radar_grid, orbit, doppler)
    out_geometry = isce.container.RadarGeometry(out_grid, orbit, doppler)

    # focus to output grid
    isce.cuda.focus.backproject(
        out,
        out_geometry,
        signal_data,
        in_geometry,
        dem,
        center_frequency,
        azimuth_res,
        kernel,
        dry_tropo_model,
    )

    # remove range carrier
    kr = 4.0 * np.pi / out_grid.wavelength
    r = np.array(out_geometry.slant_range)
    out *= np.exp(-1j * kr * r)

    info = analyze_point_target(
        out, nchip // 2, nchip // 2, nov=upsample_factor, chipsize=nchip // 2
    )
    tofloatvals(info)

    # print point target info
    print(json.dumps(info, indent=2))

    # range resolution (m)
    range_res = c / (2.0 * B)

    # range position error & -3 dB main lobe width (m)
    range_err = dr * info["range"]["offset"]
    range_width = dr * info["range"]["resolution"]

    # azimuth position error & -3 dB main lobe width (m)
    _, vel = orbit.interpolate(target_azimuth)
    azimuth_err = dt * info["azimuth"]["offset"] * np.linalg.norm(vel)
    azimuth_width = dt * info["azimuth"]["resolution"] * np.linalg.norm(vel)

    # require positioning error < resolution/128
    assert range_err < range_res / 128.0
    assert azimuth_err < azimuth_res / 128.0

    # require 3dB width in range to be <= range resolution
    assert range_width <= range_res

    # azimuth response is spread slightly by the antenna pattern so the
    # threshold is slightly higher - see
    # https://github.jpl.nasa.gov/bhawkins/nisar-notebooks/blob/master/Azimuth%20Resolution.ipynb
    assert azimuth_width <= 6.62
