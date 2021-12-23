import argparse
import os

import h5py
import iscetest
import numpy as np
import numpy.testing as npt
from nisar.workflows import h5_prep, unwrap
from nisar.workflows.unwrap_runconfig import UnwrapRunConfig


def test_unwrap_run():
    """
    Test if phase unwrapping runs smoothly
    """

    test_yaml = os.path.join(iscetest.data, "insar_test.yaml")
    with open(test_yaml) as fh_test_yaml:
        test_yaml = (
            fh_test_yaml.read()
            .replace("@ISCETEST@", iscetest.data)
            .replace("@TEST_OUTPUT@", "runw.h5")
            .replace("@TEST_PRODUCT_TYPES@", "RUNW")
        )

    # Create CLI input name space with yaml instead of filepath
    args = argparse.Namespace(run_config_path=test_yaml, log_file=False)

    # Initialize runconfig object
    runconfig = UnwrapRunConfig(args)
    runconfig.geocode_common_arg_load()

    out_paths = h5_prep.run(runconfig.cfg)

    product_path = "science/LSAR/RIFG/swaths/frequencyA/interferogram/HH"

    # Modify interferogram and correlation
    with h5py.File(out_paths["RIFG"], "a", libver="latest", swmr=True) as h_rifg:
        ds = h_rifg[os.path.join(product_path, "wrappedInterferogram")]
        nrows, ncols = ds.shape

        # Generate a ramp
        x, y = np.meshgrid(np.arange(ncols), np.arange(nrows))
        ref_igram = np.exp(1j * x)

        # Generate all 1 coherence
        ref_coh = np.ones([nrows, ncols])

        # Save reference datasets in RIFG
        h_rifg[os.path.join(product_path, "wrappedInterferogram")][:, :] = ref_igram
        h_rifg[os.path.join(product_path, "coherenceMagnitude")][:, :] = ref_coh
        h_rifg.close()

    # Run unwrapping
    unwrap.run(runconfig.cfg, out_paths["RIFG"], out_paths["RUNW"])


def test_unwrap_validate():
    """
    Validate unwrapped interferogram generated by
    the unwrapping workflow
    """
    scratch_path = "."
    group_path = "/science/LSAR/RUNW/swaths/frequencyA/interferogram/HH"

    with h5py.File(os.path.join(scratch_path, "runw.h5"), "r") as h_runw:
        # Recreate reference unwrapped phase
        h_rifg = h5py.File(os.path.join(scratch_path, "RIFG.h5"), "r")

        ref_igram = h_rifg[
            os.path.join(group_path.replace("RUNW", "RIFG"), "wrappedInterferogram")
        ]
        nrows, ncols = ref_igram.shape
        x, y = np.meshgrid(np.arange(ncols), np.arange(nrows))

        # Get processed unwrapped interferogram
        conn_comp = h_runw[f"{group_path}/connectedComponents"][()]
        uigram = h_runw[f"{group_path}/unwrappedPhase"][()]

        # Reference unwrapped and reference unwrapped interferogram to the same pixel
        uigram -= uigram[0, 0]
        x -= x[0, 0]

        # Evaluate metrics
        npt.assert_allclose(uigram, x, atol=1e-6)
        npt.assert_allclose(conn_comp, 1, atol=1e-6)


if __name__ == "__main__":
    test_unwrap_run()
    test_unwrap_validate()
