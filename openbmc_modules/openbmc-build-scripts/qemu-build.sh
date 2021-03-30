#!/bin/bash
###############################################################################
#
# This build script is for running the QEMU build in a container
#
# It expects to be run in with the qemu source present in the directory called
# '$WORKSPACE/qemu', where WORKSPACE is an environment variable.
#
# In Jenkins configure the git SCM 'Additional Behaviours', 'check-out to a sub
# directory' called 'qemu'.
#
# When building locally set WORKSPACE to be the directory above the qemu
# checkout:
#   git clone https://github.com/qemu/qemu
#   WORKSPACE=$PWD/qemu ~/openbmc-build-scripts/qemu-build.sh
#
###############################################################################
#
# Script Variables:
#  http_proxy         The HTTP address of the proxy server to connect to.
#                     Default: "", proxy is not setup if this is not set
#  WORKSPACE          Path of the workspace directory where the build will
#                     occur, and output artifacts will be produced.
#
###############################################################################
# Trace bash processing
#set -x

# Script Variables:
http_proxy=${http_proxy:-}

if [ -z ${WORKSPACE+x} ]; then
    echo "Please set WORKSPACE variable"
    exit 1
fi

# Determine the architecture
ARCH=$(uname -m)

# Docker Image Build Variables:
img_name=qemu-build

# Timestamp for job
echo "Build started, $(date)"

# Setup Proxy
if [[ -n "${http_proxy}" ]]; then
PROXY="RUN echo \"Acquire::http::Proxy \\"\"${http_proxy}/\\"\";\" > /etc/apt/apt.conf.d/000apt-cacher-ng-proxy"
fi

# Determine the prefix of the Dockerfile's base image
case ${ARCH} in
  "ppc64le")
    DOCKER_BASE="ppc64le/"
    ;;
  "x86_64")
    DOCKER_BASE=""
    ;;
  *)
    echo "Unsupported system architecture(${ARCH}) found for docker image"
    exit 1
esac

# Create the docker run script
export PROXY_HOST=${http_proxy/#http*:\/\/}
export PROXY_HOST=${PROXY_HOST/%:[0-9]*}
export PROXY_PORT=${http_proxy/#http*:\/\/*:}

cat > "${WORKSPACE}"/build.sh << EOF_SCRIPT
#!/bin/bash

set -x

# Go into the build directory
cd ${WORKSPACE}/qemu

gcc --version
git submodule update --init dtc
# disable anything that requires us to pull in X
./configure \
    --target-list=arm-softmmu \
    --disable-spice \
    --disable-docs \
    --disable-gtk \
    --disable-smartcard \
    --disable-usb-redir \
    --disable-libusb \
    --disable-sdl \
    --disable-gnutls \
    --disable-vte \
    --disable-vnc \
    --disable-werror \
    --disable-vnc-png
make clean
make -j4

EOF_SCRIPT

chmod a+x ${WORKSPACE}/build.sh

# Configure docker build
Dockerfile=$(cat << EOF
FROM ${DOCKER_BASE}ubuntu:16.04

${PROXY}

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -yy --no-install-recommends \
    bison \
    ca-certificates \
    flex \
    gcc \
    git \
    libc6-dev \
    libfdt-dev \
    libglib2.0-dev \
    libpixman-1-dev \
    make \
    python-yaml \
    python3-yaml \
    iputils-ping

RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
USER ${USER}
ENV HOME ${HOME}
EOF
)

docker build -t ${img_name} - <<< "${Dockerfile}"
if [[ "$?" -ne 0 ]]; then
  echo "Failed to build docker container."
  exit 1
fi

docker run \
    --rm=true \
    -e WORKSPACE=${WORKSPACE} \
    -w "${HOME}" \
    --user="${USER}" \
    -v "${HOME}":"${HOME}" \
    -t ${img_name} \
    ${WORKSPACE}/build.sh
