# The "runtime" image contains all runtime dependencies of the installed software.
FROM centos:7 AS runtime

# Change the default shell to a bash login shell (so that conda environments can
# be properly activated).
SHELL ["/bin/bash", "-l", "-c"]

WORKDIR /tmp

# Trying to install a package that doesn't exist should be an error.
RUN echo "skip_missing_names_on_install=False" >> /etc/yum.conf

# Add Nvidia CUDA Yum repository.
RUN yum-config-manager --add-repo https://developer.download.nvidia.com/compute/cuda/repos/rhel7/x86_64/cuda-rhel7.repo

# CUDA version string, in "<major>-<minor>" format.
ARG cuda_version

# Install runtime dependencies with Yum.
RUN yum update -y \
 && yum install -y \
    cuda-cudart-$cuda_version \
    libcufft-$cuda_version \
 && yum clean all \
 && rm -rf /var/cache/yum

# Install Mamba package manager. The Mambaforge distribution is a stripped-down
# version of mamba (similar to Miniconda) that uses `conda-forge` as the default
# (and only) channel.
ADD https://github.com/conda-forge/miniforge/releases/latest/download/Mambaforge-Linux-x86_64.sh .
ARG mamba_prefix=/opt/mambaforge
RUN sh Mambaforge-Linux-x86_64.sh -b -p $mamba_prefix
ENV PATH=$mamba_prefix/bin:$PATH
ENV MAMBA_NO_BANNER=1
RUN mamba init

# Create an empty conda environment. The `base` environment contains some
# pre-installed packages that could cause conflicts or mask missing dependencies.
RUN mamba create -n isce3

# Package version specifications for several dependencies. These could be empty
# strings, in which case the version is unconstrained.
ARG eigen_version
ARG gcc_version
ARG gdal_version
ARG python_version

# Install runtime dependencies with Mamba.
RUN mamba update -y mamba \
 && mamba install -n isce3 -y \
    eigen=$eigen_version \
    fftw \
    gdal=$gdal_version \
    h5py \
    hdf5 \
    libstdcxx-ng=$gcc_version \
    numpy \
    pyre \
    python=$python_version \
    ruamel.yaml \
    scipy \
    shapely \
    sysroot_linux-64=2.17 \
    yamale \
 && mamba clean -afy

# Add isce3 installation path(s) to environment.
ENV ISCE3_PREFIX=/opt/isce3
ENV PATH=$ISCE3_PREFIX/bin:$PATH \
    LD_LIBRARY_PATH=$ISCE3_PREFIX/lib64:$LD_LIBRARY_PATH \
    PYTHONPATH=$ISCE3_PREFIX/packages:$PYTHONPATH

# Change the default ENTRYPOINT so that commands run by the container are
# executed inside the `isce3` conda environment.
ENTRYPOINT ["mamba", "run", "-n", "isce3", "--no-capture-output"]

# The "devel" image inherits from the "runtime" image and includes all of the
# build-time and testing requirements of the software.
FROM runtime AS devel

ARG cuda_version

# Install build & test dependencies with Yum.
RUN yum update -y \
 && yum install -y \
    cuda-cudart-devel-$cuda_version \
    cuda-nvcc-$cuda_version \
    libcufft-devel-$cuda_version \
 && yum clean all \
 && rm -rf /var/cache/yum

ARG cmake_version
ARG gcc_version
ARG pybind11_version

# Install build & test dependencies with Mamba.
RUN mamba update -y mamba \
 && mamba install -n isce3 -y \
    cmake=$cmake_version \
    gxx_linux-64=$gcc_version \
    gmock \
    gtest \
    ninja \
    pybind11=$pybind11_version \
    pytest \
 && mamba clean -afy

# Add CUDA bin path to environment. Some older CUDA packages don't create a
# version-agnostic symlink, so we create one manually.
ENV CUDA_PREFIX=/usr/local/cuda
RUN ln -s /usr/local/cuda-${cuda_version//-/.} $CUDA_PREFIX
ENV PATH=$CUDA_PREFIX/bin:$PATH

FROM devel AS builder

# Copy the source directory from the host. (The build context must be the root
# directory of the repository for this to work properly.)
COPY . isce3

WORKDIR isce3

# CMake build type ("Release", "Debug", etc).
ARG build_type

# Run CMake.
RUN mamba activate isce3 \
 && cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=$build_type \
    -DCMAKE_CUDA_HOST_COMPILER=$CXX \
    -DCMAKE_CXX_FLAGS="-fdiagnostics-color=auto" \
    -DCMAKE_INSTALL_PREFIX=$ISCE3_PREFIX \
    -DWITH_CUDA=ON \
    .

FROM builder AS installer

WORKDIR build

# Build isce3.
RUN mamba activate isce3 && cmake --build .

FROM installer AS tester

# Install isce3.
RUN mamba activate isce3 && cmake --build . --target install

FROM runtime AS isce3

# Finally, copy the installed components onto the runtime image.
COPY --from=tester $ISCE3_PREFIX $ISCE3_PREFIX
