#!/bin/bash -xe
#
# Build the required docker image to run rootfs_size.py
#
# Script Variables:
#   DOCKER_IMG_NAME:  <optional, the name of the docker image to generate>
#                     default is openbmc/ubuntu-rootfs-size
#   DISTRO:           <optional, the distro to build a docker image against>
#                     default is ubuntu:bionic

set -uo pipefail

DOCKER_IMG_NAME=${DOCKER_IMG_NAME:-"openbmc/ubuntu-rootfs-size"}
DISTRO=${DISTRO:-"ubuntu:bionic"}

# Determine the architecture
ARCH=$(uname -m)
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

################################# docker img # #################################

if [[ "${DISTRO}" == "ubuntu"* ]]; then
Dockerfile=$(cat << EOF
FROM ${DOCKER_BASE}${DISTRO}

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get install -yy \
    python3 \
    python3-dev\
    python3-yaml \
    python3-mako \
    python3-pip \
    python3-setuptools \
    curl \
    git \
    wget \
    sudo \
    squashfs-tools

# Final configuration for the workspace
RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
RUN mkdir -p $(dirname ${HOME})
RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
RUN sed -i '1iDefaults umask=000' /etc/sudoers
RUN echo "${USER} ALL=(ALL) NOPASSWD: ALL" >>/etc/sudoers

RUN /bin/bash
EOF
)
fi
################################# docker img # #################################

# Build above image
docker build --network=host -t ${DOCKER_IMG_NAME} - <<< "${Dockerfile}"
