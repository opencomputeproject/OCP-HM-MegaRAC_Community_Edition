#!/bin/bash -xe

# This script is for running rootfs_size.py in Jenkins using docker.
#
# This script will build a docker container which will then be used to build
# and run the rootfs_size.py script.
#
#   WORKSPACE:       Required, location of unit test scripts and repository
#                    code to test
#   SQUASHFS_FILE:   Required, The squashfs file name to run rootfs_size
#                    against
#   DISTRO:          Optional, docker base image (ubuntu or fedora)
#   DOCKER_IMG_NAME: Optional, default is openbmc/ubuntu-rootfs-size

# Trace bash processing. Set -e so when a step fails, we fail the build
set -uo pipefail

# Default variables
DOCKER_IMG_NAME=${DOCKER_IMG_NAME:-"openbmc/ubuntu-rootfs-size"}
DISTRO=${DISTRO:-ubuntu:bionic}
OBMC_BUILD_SCRIPTS="openbmc-build-scripts"
OBMC_TOOLS="openbmc-tools"
ROOTFS_SIZE_PY_DIR="edtanous"
ROOTFS_SIZE_PY="rootfs_size.py"

# Timestamp for job
echo "rootfs_size build started, $(date)"

if [[ "${DISTRO}" == "fedora" ]]; then
    echo "Distro (${DISTRO}) not supported, running as ubuntu"
    DISTRO="ubuntu:bionic"
fi

# Check workspace, build scripts exist
if [ ! -d "${WORKSPACE}" ]; then
    echo "Workspace(${WORKSPACE}) doesn't exist, exiting..."
    exit 1
fi

if [ ! -e "${WORKSPACE}/${SQUASHFS_FILE}" ]; then
    echo "${WORKSPACE}/${SQUASHFS_FILE} doesn't exist, exiting..."
    exit 1
fi

if [ ! -d "${WORKSPACE}/${OBMC_BUILD_SCRIPTS}" ]; then
    echo "Clone (${OBMC_BUILD_SCRIPTS}) in ${WORKSPACE}..."
    git clone https://gerrit.openbmc-project.xyz/openbmc/${OBMC_BUILD_SCRIPTS} ${WORKSPACE}/${OBMC_BUILD_SCRIPTS}
fi

if [ ! -d "${WORKSPACE}/${OBMC_TOOLS}" ]; then
    echo "Clone (${OBMC_TOOLS}) in ${WORKSPACE}..."
    git clone https://gerrit.openbmc-project.xyz/openbmc/${OBMC_TOOLS} ${WORKSPACE}/${OBMC_TOOLS}
fi

# Copy rootfs_size.py script into workspace
cp ${WORKSPACE}/${OBMC_TOOLS}/${ROOTFS_SIZE_PY_DIR}/${ROOTFS_SIZE_PY} \
${WORKSPACE}/${ROOTFS_SIZE_PY}
chmod a+x ${WORKSPACE}/${ROOTFS_SIZE_PY}

# Configure docker build
cd ${WORKSPACE}/${OBMC_BUILD_SCRIPTS}
echo "Building docker image with build-rootfs-size-docker.sh"

# Export input env variables
export DOCKER_IMG_NAME
export DISTRO
./build-rootfs-size-docker.sh

# Run the docker container with the rootfs_size execution script
echo "Executing docker image"
docker run --cap-add=sys_admin --rm=true \
    --network host \
    --privileged=true \
    -u "$USER" \
    -w "${WORKSPACE}" -v "${WORKSPACE}":"${WORKSPACE}" \
    -t ${DOCKER_IMG_NAME} \
    "${WORKSPACE}"/${ROOTFS_SIZE_PY} --build_dir ${WORKSPACE}/ --squashfs_file ${SQUASHFS_FILE}

# Timestamp for build
echo "rootfs_size build completed, $(date)"
