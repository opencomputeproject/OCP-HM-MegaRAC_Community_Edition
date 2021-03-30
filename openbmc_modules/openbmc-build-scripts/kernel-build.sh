#!/bin/bash

# This build script is for running the Jenkins builds using docker.
#

# Trace bash processing
#set -x

# Default variables
WORKSPACE=${WORKSPACE:-${HOME}/${RANDOM}${RANDOM}}
http_proxy=${http_proxy:-}

# Timestamp for job
echo "Build started, $(date)"

# Configure docker build
if [[ -n "${http_proxy}" ]]; then
PROXY="RUN echo \"Acquire::http::Proxy \\"\"${http_proxy}/\\"\";\" > /etc/apt/apt.conf.d/000apt-cacher-ng-proxy"
fi

Dockerfile=$(cat << EOF
FROM ubuntu:latest

${PROXY}

ENV DEBIAN_FRONTEND noninteractive 
RUN apt-get update && apt-get install -yy \
	bc \
	build-essential \
	git \
	gcc-powerpc64le-linux-gnu \
	software-properties-common \
	libssl-dev \
	bison \
	flex \
	iputils-ping

RUN apt-add-repository -y multiverse && apt-get update && apt-get install -yy \
	dwarves \
	sparse

RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}

USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)

# Build the docker container
docker build -t linux-build/ubuntu - <<< "${Dockerfile}"
if [[ "$?" -ne 0 ]]; then
  echo "Failed to build docker container."
  exit 1
fi

# Create the docker run script
export PROXY_HOST=${http_proxy/#http*:\/\/}
export PROXY_HOST=${PROXY_HOST/%:[0-9]*}
export PROXY_PORT=${http_proxy/#http*:\/\/*:}

mkdir -p ${WORKSPACE}

cat > "${WORKSPACE}"/build.sh << EOF_SCRIPT
#!/bin/bash

set -x
set -e -o pipefail

cd ${WORKSPACE}

# Go into the linux directory (the script will put us in a build subdir)
cd linux

# Record the version in the logs
powerpc64le-linux-gnu-gcc --version

# Build kernel prep
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make mrproper

# Build kernel
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make -s pseries_le_defconfig
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make -s -j$(nproc)

# Build kernel with debug
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make -s pseries_le_defconfig
echo "CONFIG_DEBUG_INFO=y" >> .config
# Enable virtio-net driver for QEMU virtio-net-pci driver
echo "CONFIG_VIRTIO=y" >> .config
echo "CONFIG_VIRTIO_NET=y" >> .config
echo "CONFIG_VIRTIO_PCI=y" >> .config
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make olddefconfig
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make -j$(nproc) -s C=2 CF=-D__CHECK_ENDIAN__ 2>&1 | gzip > sparse.log.gz
pahole vmlinux 2>&1 | gzip > structs.dump.gz

EOF_SCRIPT

chmod a+x ${WORKSPACE}/build.sh

# Run the docker container, execute the build script we just built
docker run --cap-add=sys_admin --net=host --rm=true -e WORKSPACE=${WORKSPACE} --user="${USER}" \
  -w "${WORKSPACE}" -v "${WORKSPACE}":"${WORKSPACE}" -t linux-build/ubuntu ${WORKSPACE}/build.sh

result=${?}

# Timestamp for build
echo "Build completed, $(date)"

exit ${result}
