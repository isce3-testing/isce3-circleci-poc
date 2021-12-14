# The "runtime" image contains all runtime dependencies of the installed software.
FROM ubuntu:focal AS runtime

ARG cuda_version

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
    python3 \
    python3-gdal \
    python3-pip \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Install runtime dependencies with pip.
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

ARG cuda_version

# Add Kitware APT repository.
ADD https://apt.kitware.com/keys/kitware-archive-latest.asc .
RUN apt-key add kitware-archive-latest.asc
RUN apt-add-repository "deb https://apt.kitware.com/ubuntu/ focal main"

# Install build & test dependencies with APT.
RUN apt-get update \
 && apt-get install -y \
    cmake \
    cuda \
    cuda-cudart-dev-$cuda_version \
    cuda-nvcc-$cuda_version \
    googletest \
    libcufft-dev-$cuda_version \
    ninja-build \
    python3-dev \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Install build & test dependencies with pip.
RUN pip install --no-cache-dir \
    pybind11 \
    pytest

# Add CUDA bin path to environment.
ENV CUDA_PREFIX=/usr/local/cuda
ENV PATH=$CUDA_PREFIX/bin:$PATH

# # Copy the source directory from the host into the container.
# COPY . isce3
# WORKDIR isce3
#
# # Configure.
# RUN cmake -G=Ninja -B=build \
#     -DCMAKE_INSTALL_PREFIX=$ISCE3_PREFIX \
#     -DWITH_CUDA=ON \
#     .
#
# WORKDIR build
#
# # Build.
# RUN cmake --build .
#
# # Install.
# RUN cmake --build . --target=install

# # Run unit tests.
# RUN ctest -E="stage_dem" -j=$(nproc) --output-on-failure
