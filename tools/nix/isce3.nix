{ stdenv, lib
, addOpenGLRunpath
, buildPythonPackage
, cmake
, cudatoolkit
, cython
, eigen
, fftw
, fftwFloat
, gdal
, gtest
, h5py
, hdf5-cpp
, lcov
, ninja
, numpy
, pybind11
, pyre
, pytest
, python3
, ruamel_yaml
, yamale

, callPackage

# Options
, config # system configuration
, withCuda ? stdenv.isLinux
, withCython ? false
, withCtestOutput ? true
}:

stdenv.mkDerivation rec {
  pname = "isce3";
  version = lib.removeSuffix "\n" (builtins.readFile ../../VERSION.txt);

  outputs = [ "out" ]
    ++ lib.optional withCtestOutput "ctest"
    ;

  nativeBuildInputs = [
    cmake
    ninja
  ] ++ lib.optional withCython cython
    ++ lib.optional (withCython || doInstallCheck) python3
    ++ lib.optionals withCuda [ addOpenGLRunpath cudatoolkit ]
  ;

  buildInputs = [
    eigen
    fftw
    fftwFloat
    gdal
    gtest
    hdf5-cpp
    pybind11
    pyre
    pytest
    (python3.withPackages (ps: with ps; [
      gdal
      (h5py.override {
        hdf5 = hdf5-cpp;
      })
      numpy
      ruamel_yaml
      scipy
      yamale
    ]))
  ];

  CUDA_VISIBLE_DEVICES=2;

  src = lib.cleanSourceWith {
    src = lib.cleanSource ../..;
    filter = name: type: let
      basename = baseNameOf (toString name);
    in !(
      basename == "build" ||
      lib.hasSuffix ".nix" basename
    );
  };

  cmakeFlags = [
    "-DCMAKE_BUILD_WITH_INSTALL_RPATH=YES" # needed for ctests
    "-DISCE3_FETCH_DEPS=NO"
    "-DHAVE_PYRE=y"
    "-DISCE_PACKAGESDIR=${python3.sitePackages}"
    "-DWITH_CUDA=${lib.boolToString withCuda}"
    "-DISCE3_WITH_CYTHON=${lib.boolToString withCython}"
    "-DCMAKE_CUDA_ARCHITECTURES=70-real"
    "-DCMAKE_BUILD_TYPE=Debug"
  ];

  enableParallelBuilding = true;

  postInstall = lib.optionalString withCuda ''
    addOpenGLRunpath $out/lib/libisce3-cuda.so
  '';

  # isce3 requires install before running ctest
  doCheck = false;
  doInstallCheck = true;
  preInstallCheck = ''
    export PYTHONPATH=$out/${python3.sitePackages}:$PYTHONPATH
  '';
  installCheckPhase = let
    ctestExcludes = [
      "import_pyre" # faulty (TODO fix)
      "stage_dem" # requires credentials
      "pybind_nisar.workflows.resample_slc" # TODO investigate
    ];
  in ''
    runHook preInstallCheck
    ctest --output-on-failure -E '(${lib.concatStringsSep "|" ctestExcludes})'
    runHook postInstallCheck
  '';

  postInstallCheck = lib.optionalString withCtestOutput ''
    mkdir -p $ctest
    cp Testing/*/Test.xml $ctest
  '';

  meta = with lib; {
    homepage = "https://github.com/isce-framework/isce3";
    description = "The InSAR Scientific Computing Environment, cubed";
    license = licenses.asl20;
    platform = platforms.unix;
  };
}
