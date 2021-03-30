#!/bin/bash

# This build script is for running the Jenkins builds using docker.
#
# It expects a variable as part of Jenkins build job matrix:
#   distro = fedora|ubuntu
#   WORKSPACE =

# Trace bash processing
set -x

# Default variables
distro=${distro:-ubuntu}
WORKSPACE=${WORKSPACE:-${HOME}/${RANDOM}${RANDOM}}
http_proxy=${http_proxy:-}

# Timestamp for job
echo "Build started, $(date)"

# Configure docker build
if [[ "${distro}" == fedora ]];then

  if [[ -n "${http_proxy}" ]]; then
    PROXY="RUN echo \"proxy=${http_proxy}\" >> /etc/dnf/dnf.conf"
  fi

  Dockerfile=$(cat << EOF
FROM fedora:latest

${PROXY}

RUN dnf --refresh install -y \
	bc \
	findutils \
	git \
	gcc \
	gcc-arm-linux-gnu \
	hostname \
	make \
	uboot-tools xz

RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}

USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)

elif [[ "${distro}" == ubuntu ]]; then
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
	gcc-arm-linux-gnueabi \
	binutils-arm-linux-gnueabi \
	libssl-dev \
	bison \
	flex \
	u-boot-tools

RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}

USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)
fi

# Build the docker container
docker build -t linux-aspeed/${distro} - <<< "${Dockerfile}"
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

cd ${WORKSPACE}

# Go into the linux-aspeed directory (the script will put us in a build subdir)
cd linux-aspeed

# Configure a build
if [ "${distro}" == "fedora" ]; then
  CROSS_COMPILER="arm-linux-gnu-"
else
  CROSS_COMPILER="arm-linux-gnueabi-"
fi

# Record the version in the logs
$\{CROSS_COMPILER}gcc --version

ARCH=arm CROSS_COMPILE=\${CROSS_COMPILER} make aspeed_defconfig
ARCH=arm CROSS_COMPILE=\${CROSS_COMPILER} make -j$(($(nproc) / 4))

# build palmetto image
ARCH=arm CROSS_COMPILE=\${CROSS_COMPILER} make aspeed-bmc-opp-palmetto.dtb
cat arch/arm/boot/zImage arch/arm/boot/dts/aspeed-bmc-opp-palmetto.dtb > palmetto-zimage
./scripts/mkuboot.sh -A arm -O linux -C none  -T kernel -a 0x40008000 -e 0x40008000 -d-e 0x40008000 -n obmc-palm-\$(date +%Y%m%d%H%M) -d palmetto-zimage uImage.palmetto

EOF_SCRIPT

chmod a+x ${WORKSPACE}/build.sh

# Run the docker container, execute the build script we just built
docker run --cap-add=sys_admin --net=host --rm=true -e WORKSPACE=${WORKSPACE} --user="${USER}" \
  -w "${HOME}" -v "${HOME}":"${HOME}" -t linux-aspeed/${distro} ${WORKSPACE}/build.sh

# Timestamp for build
echo "Build completed, $(date)"

