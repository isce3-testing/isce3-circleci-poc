# The "runtime" image contains all runtime dependencies of the installed software.
FROM ubuntu:20.04 AS runtime

WORKDIR /tmp

ARG DEBIAN_FRONTEND=noninteractive

# Install prerequisite tools.
RUN apt-get update \
 && apt-get install -y \
    git \
    gnupg \
    software-properties-common \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Add Nvidia CUDA APT repository.
ADD https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-ubuntu2004.pin .
RUN cp cuda-ubuntu2004.pin /etc/apt/preferences.d/cuda-repository-pin-600
RUN apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/7fa2af80.pub
RUN add-apt-repository "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/ /"

# CUDA version string, in "<major>-<minor>" format.
ARG cuda_version

# Python version string, in "<major>.<minor>" format.
ARG python_version=3

# Install runtime dependencies with APT.
RUN apt-get update \
 && apt-get install -y \
    build-essential \
    cuda-cudart-$cuda_version \
    libcufft-$cuda_version \
    libeigen3-dev \
    libfftw3-dev \
    libfftw3-double3 \
    libfftw3-single3 \
    libgdal-dev \
    libhdf5-dev \
    python$python_version \
    python3-gdal \
    python3-pip \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Install runtime dependencies with pip. We must build h5py from source to
# ensure binary compatibility with the installed version of libhdf5.
RUN pip install --no-cache-dir --no-binary=h5py \
    h5py \
    numpy \
    ruamel.yaml \
    scipy \
    shapely \
    yamale

# Add isce3 installation path(s) to environment.
ENV ISCE3_PREFIX=/opt/isce3
ENV PATH=$ISCE3_PREFIX/bin:$PATH \
    LD_LIBRARY_PATH=$ISCE3_PREFIX/lib:$LD_LIBRARY_PATH \
    PYTHONPATH=$ISCE3_PREFIX/packages:$PYTHONPATH

# The "devel" image inherits from the "runtime" image and includes all of the
# build-time and testing requirements of the software.
FROM runtime AS devel

ARG DEBIAN_FRONTEND=noninteractive

# Add Kitware APT repository.
ADD https://apt.kitware.com/keys/kitware-archive-latest.asc .
RUN apt-key add kitware-archive-latest.asc
RUN apt-add-repository "deb https://apt.kitware.com/ubuntu/ focal main"

ARG cuda_version
ARG python_version

# Install build & test dependencies with APT.
RUN apt-get update \
 && apt-get install -y \
    cmake \
    cuda-cudart-dev-$cuda_version \
    cuda-nvcc-$cuda_version \
    googletest \
    libcufft-dev-$cuda_version \
    ninja-build \
    python$python_version-dev \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Install build & test dependencies with pip.
RUN pip install --no-cache-dir \
    pybind11 \
    pytest

# Add CUDA bin path to environment.
ENV CUDA_PREFIX=/usr/local/cuda
ENV PATH=$CUDA_PREFIX/bin:$PATH

FROM devel AS builder

# Copy the source directory from the host. (The build context must be the root
# directory of the repository for this to work properly.)
COPY . isce3

WORKDIR isce3

# CMake build type ("Release", "Debug", etc).
ARG build_type

# Run CMake.
RUN cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=$build_type \
    -DCMAKE_CXX_FLAGS="-fdiagnostics-color=auto" \
    -DCMAKE_INSTALL_PREFIX=$ISCE3_PREFIX \
    -DWITH_CUDA=ON \
    .

FROM builder AS installer

WORKDIR build

# Build isce3.
RUN cmake --build .

FROM installer AS tester

# Install isce3.
RUN cmake --build . --target install

FROM runtime AS isce3

# Finally, copy the installed components onto the runtime image.
COPY --from=tester $ISCE3_PREFIX $ISCE3_PREFIX
