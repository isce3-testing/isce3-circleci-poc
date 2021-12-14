# The "runtime" image contains all runtime dependencies of the installed software.
FROM centos:7 AS runtime

ARG cuda_version
ARG gcc_version
ARG python_version

# Use a login bash shell as the default shell so that $HOME/.bashrc is sourced
# on each shell invocation. This is useful for interacting with conda environments.
SHELL ["/bin/bash", "-lc"]

WORKDIR /tmp

# Trying to install a package that doesn't exist should be an error.
RUN echo "skip_missing_names_on_install=False" >> /etc/yum.conf

# Add Nvidia CUDA Yum repository.
RUN yum-config-manager --add-repo https://developer.download.nvidia.com/compute/cuda/repos/rhel7/x86_64/cuda-rhel7.repo

# Install runtime dependencies with Yum.
RUN yum update -y \
 && yum install -y \
    cuda-cudart-$cuda_version \
    libcufft-$cuda_version \
 && yum clean all \
 && rm -rf /var/cache/yum

# Install Mamba package manager.
ADD https://github.com/conda-forge/miniforge/releases/latest/download/Mambaforge-Linux-x86_64.sh .
RUN chmod +x Mambaforge-Linux-x86_64.sh
RUN ./Mambaforge-Linux-x86_64.sh -b
RUN $HOME/mambaforge/bin/mamba init

# Install runtime dependencies with Mamba.
RUN mamba install -y \
    eigen \
    fftw \
    gdal \
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
    LD_LIBRARY_PATH=$ISCE3_PREFIX/lib:$LD_LIBRARY_PATH \
    PYTHONPATH=$ISCE3_PREFIX/packages:$PYTHONPATH

# The "devel" image inherits from the "runtime" image and includes all of the
# build-time and testing requirements of the software.
FROM runtime AS devel

ARG cuda_version
ARG gcc_version

# Install build & test dependencies with Yum.
RUN yum update -y \
 && yum install -y \
    cuda-cudart-devel-$cuda_version \
    cuda-nvcc-$cuda_version \
    libcufft-devel-$cuda_version \
 && yum clean all \
 && rm -rf /var/cache/yum

# Install build & test dependencies with Mamba.
RUN mamba install -y \
    cmake \
    gxx_linux-64=$gcc_version \
    gmock \
    gtest \
    ninja \
    pybind11 \
    pytest \
 && mamba clean -afy

# Add CUDA bin path to environment.
ENV CUDA_PREFIX=/usr/local/cuda
ENV PATH=$CUDA_PREFIX/bin:$PATH

# Copy the source directory from the host into the container.
COPY . isce3
WORKDIR isce3
