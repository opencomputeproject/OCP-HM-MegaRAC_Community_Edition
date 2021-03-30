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
	iputils-ping \
	bison \
	flex

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
docker build -t trace-linux-build/ubuntu - <<< "${Dockerfile}"
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

set -xe

cd ${WORKSPACE}

# Go into the linux directory (the script will put us in a build subdir)
cd linux

# Record the version in the logs
powerpc64le-linux-gnu-gcc --version

# Build kernel prep
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make mrproper

# Generate the config
echo "CONFIG_PPC_PSERIES=y" >> fragment.config
echo "CONFIG_PPC_POWERNV=y" >> fragment.config
echo "CONFIG_MTD=y" >> fragment.config
echo "CONFIG_MTD_BLOCK=y" >> fragment.config
echo "CONFIG_MTD_POWERNV_FLASH=y" >> fragment.config
ARCH=powerpc scripts/kconfig/merge_config.sh \
    arch/powerpc/configs/ppc64_defconfig \
    arch/powerpc/configs/le.config \
    fragment.config

# Ensure config is up to date and no questions will be asked
yes "" | ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make oldconfig

# Build kernel
ARCH=powerpc CROSS_COMPILE=powerpc64le-linux-gnu- make -j$(nproc) vmlinux

EOF_SCRIPT

chmod a+x ${WORKSPACE}/build.sh

# Run the docker container, execute the build script we just built
docker run --cap-add=sys_admin --net=host --rm=true -e WORKSPACE=${WORKSPACE} --user="${USER}" \
  -w "${WORKSPACE}" -v "${WORKSPACE}":"${WORKSPACE}" -t trace-linux-build/ubuntu ${WORKSPACE}/build.sh

result=${?}

# Timestamp for build
echo "Build completed, $(date)"

exit ${result}
