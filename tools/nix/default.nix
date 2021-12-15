{ pkgs ? import (fetchTarball {
    # Remember to update the cache key in .circleci/config.yml when bumping!
    url = "https://github.com/NixOS/nixpkgs/archive/5f5bc79a1bf33493a5fb0e599c4f206fc0cb491e.tar.gz";
    sha256 = "1371nxfy4l2r190yf3ayzg1fcrasrn9laff0khmgwfsxrcam9a9h";
  }) {}
}:
with pkgs;
python3.pkgs.callPackage ./isce3.nix {
  cudatoolkit = cudatoolkit_11;
  pyre = python3.pkgs.callPackage ./pyre.nix {};
}
