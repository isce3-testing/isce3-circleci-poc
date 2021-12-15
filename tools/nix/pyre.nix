{ lib, stdenv
, fetchFromGitHub
, cmake
, ninja
, numpy
, pybind11
, python
}:

stdenv.mkDerivation rec {
  pname = "pyre";
  version = "1.11.0";

  outputs = [ "out" "dev" ];

  src = fetchFromGitHub {
    owner = pname;
    repo = pname;
    rev = "v${version}";
    sha256 = "1yksswiidjljv84rwzk35770if5jwvgiwydibn7q2z7rm74alxcb";
  };

  nativeBuildInputs = [
    cmake
    ninja
  ];
  buildInputs = [
    numpy
    pybind11
  ];

  cmakeFlags = [
    "-DBUILD_TESTING=y"
    "-DPYRE_DEST_PACKAGES=${python.sitePackages}"
    "-DPYRE_VERSION=${version}"
  ];

  postInstall = ''
    moveToOutput "share/cmake" "$dev"

    moveToOutput "bin" "$out"
    moveToOutput "defaults" "$out"
    moveToOutput "lib/${python.libPrefix}" "$out"
  '';

  meta = with lib; {
    homepage = "https://pyre.orthologue.com";
    description = "An open source framework for scientific applications";
    license = licenses.bsd3;
    platforms = platforms.unix;
  };
}
