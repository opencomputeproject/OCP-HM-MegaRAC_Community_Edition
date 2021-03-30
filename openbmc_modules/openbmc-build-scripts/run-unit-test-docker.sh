#!/bin/bash -xe

# This build script is for running the Jenkins unit test builds using docker.
#
# This script will build a docker container which will then be used to build
# and test the input UNIT_TEST_PKG. The docker container will be pre-populated
# with the most used OpenBMC repositories (phosphor-dbus-interfaces, sdbusplus,
# phosphor-logging, ...). This allows the use of docker caching
# capabilities so the dependent repositories are only built once per update
# to their corresponding repository. If a BRANCH parameter is input then the
# docker container will be pre-populated with the latest code from that input
# branch. If the branch does not exist in the repository, then master will be
# used.
#
#   UNIT_TEST_PKG:   Required, repository which has been extracted and is to
#                    be tested
#   WORKSPACE:       Required, location of unit test scripts and repository
#                    code to test
#   DISTRO:          Optional, docker base image (ubuntu or fedora)
#   BRANCH:          Optional, branch to build from each of the
#                    openbmc repositories. default is master, which will be
#                    used if input branch not provided or not found
#   DOCKER_IMG_NAME: Optional, default is openbmc/ubuntu-unit-test-master with a
#                    -$BRANCH replacing -master if $BRANCH provided
#   dbus_sys_config_file: Optional, with the default being
#                         `/usr/share/dbus-1/system.conf`
#   NO_FORMAT_CODE:  Optional, do not run format-code.sh

# Trace bash processing. Set -e so when a step fails, we fail the build
set -uo pipefail

# Default variables
BRANCH=${BRANCH:-"master"}
DOCKER_IMG_NAME=${DOCKER_IMG_NAME:-"openbmc/ubuntu-unit-test-${BRANCH}"}
DISTRO=${DISTRO:-ubuntu:focal}
OBMC_BUILD_SCRIPTS="openbmc-build-scripts"
UNIT_TEST_PY_DIR="scripts"
UNIT_TEST_PY="unit-test.py"
FORMAT_CODE_SH="format-code.sh"
DBUS_UNIT_TEST_PY="dbus-unit-test.py"
TEST_ONLY="${TEST_ONLY:-}"
DBUS_SYS_CONFIG_FILE=${dbus_sys_config_file:-"/usr/share/dbus-1/system.conf"}
MAKEFLAGS="${MAKEFLAGS:-""}"
DOCKER_WORKDIR="${DOCKER_WORKDIR:-$WORKSPACE}"
NO_FORMAT_CODE="${NO_FORMAT_CODE:-}"

# Timestamp for job
echo "Unit test build started, $(date)"

if [[ "${DISTRO}" == "fedora" ]]; then
    echo "Distro (${DISTRO}) not supported, running as ubuntu"
    DISTRO="ubuntu:latest"
fi

# Check workspace, build scripts, and package to be unit tested exists
if [ ! -d "${WORKSPACE}" ]; then
    echo "Workspace(${WORKSPACE}) doesn't exist, exiting..."
    exit 1
fi
if [ ! -d "${WORKSPACE}/${OBMC_BUILD_SCRIPTS}" ]; then
    echo "Package(${OBMC_BUILD_SCRIPTS}) not found in ${WORKSPACE}, exiting..."
    exit 1
fi
if [ ! -d "${WORKSPACE}/${UNIT_TEST_PKG}" ]; then
    echo "Package(${UNIT_TEST_PKG}) not found in ${WORKSPACE}, exiting..."
    exit 1
fi

# Copy unit test script into workspace
cp ${WORKSPACE}/${OBMC_BUILD_SCRIPTS}/${UNIT_TEST_PY_DIR}/${UNIT_TEST_PY} \
${WORKSPACE}/${UNIT_TEST_PY}
chmod a+x ${WORKSPACE}/${UNIT_TEST_PY}

# Copy dbus unit test script into workspace
cp ${WORKSPACE}/${OBMC_BUILD_SCRIPTS}/${UNIT_TEST_PY_DIR}/${DBUS_UNIT_TEST_PY} \
${WORKSPACE}/${DBUS_UNIT_TEST_PY}
chmod a+x ${WORKSPACE}/${DBUS_UNIT_TEST_PY}

# Copy format code script into workspace
cp ${WORKSPACE}/${OBMC_BUILD_SCRIPTS}/${UNIT_TEST_PY_DIR}/${FORMAT_CODE_SH} \
${WORKSPACE}/${FORMAT_CODE_SH}
chmod a+x ${WORKSPACE}/${FORMAT_CODE_SH}

# Configure docker build
cd ${WORKSPACE}/${OBMC_BUILD_SCRIPTS}
echo "Building docker image with build-unit-test-docker.sh"
# Export input env variables
export DOCKER_IMG_NAME
export DISTRO
export BRANCH
./build-unit-test-docker.sh

# Unit test and parameters
UNIT_TEST="${DOCKER_WORKDIR}/${UNIT_TEST_PY},-w,${DOCKER_WORKDIR},\
-p,${UNIT_TEST_PKG},-b,$BRANCH,-v${TEST_ONLY:+,-t}${NO_FORMAT_CODE:+,-n}"

# Run the docker unit test container with the unit test execution script
echo "Executing docker image"
docker run --cap-add=sys_admin --rm=true \
    --network host \
    --privileged=true \
    -u "$USER" \
    -w "${DOCKER_WORKDIR}" -v "${WORKSPACE}":"${DOCKER_WORKDIR}" \
    -e "MAKEFLAGS=${MAKEFLAGS}" \
    -t ${DOCKER_IMG_NAME} \
    "${DOCKER_WORKDIR}"/${DBUS_UNIT_TEST_PY} -u ${UNIT_TEST} \
    -f ${DBUS_SYS_CONFIG_FILE}

# Timestamp for build
echo "Unit test build completed, $(date)"

# Clean up copied scripts.
rm ${WORKSPACE}/${UNIT_TEST_PY}
rm ${WORKSPACE}/${DBUS_UNIT_TEST_PY}
rm ${WORKSPACE}/${FORMAT_CODE_SH}

