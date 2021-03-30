#!/bin/bash

# This build script is for running the Jenkins builds using docker.
#

# Debug
if grep -q debug <<< $@; then
	set -x
fi
set -o errexit
set -o pipefail
set -o nounset

# Default variables
WORKSPACE=${WORKSPACE:-${HOME}/${RANDOM}${RANDOM}}
http_proxy=${http_proxy:-}
ENDIANESS=${ENDIANESS:-le}
PROXY=""

usage(){
	cat << EOF_USAGE
Usage: $0 [options]

Options:
--endianess <le|be>	build an LE or BE initramfs

Short Options:
-e			same as --endianess

EOF_USAGE
	exit 1
}

# Arguments
CMD_LINE=$(getopt -o d,e: --longoptions debug,endianess: -n "$0" -- "$@")
eval set -- "${CMD_LINE}"

while true ; do
	case "${1}" in
		-e|--endianess)
			if [[ "${2,,}" == "be" ]]; then
				ENDIANESS=""
			fi
			shift 2
			;;
		-d|--debug)
			set -x
			shift
			;;
		--)
			shift
			break
			;;
		*)
			usage
			;;
	esac
done

# Timestamp for job
echo "Build started, $(date)"

# Configure docker build
if [[ -n "${http_proxy}" ]]; then
	PROXY="RUN echo \"Acquire::http::Proxy \\"\"${http_proxy}/\\"\";\" > /etc/apt/apt.conf.d/000apt-cacher-ng-proxy"
fi

Dockerfile=$(cat << EOF
FROM ubuntu:18.04

${PROXY}

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -yy \
	bc \
	build-essential \
	ccache \
	cpio \
	git \
	python \
	unzip \
	wget \
	rsync \
	iputils-ping \
	locales

RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}

RUN locale-gen en_AU.utf8

USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)

# Build the docker container
docker build -t initramfs-build/ubuntu - <<< "${Dockerfile}"
if [[ "$?" -ne 0 ]]; then
	echo "Failed to build docker container."
	exit 1
fi

# Create the docker run script
export PROXY_HOST=${http_proxy/#http*:\/\/}
export PROXY_HOST=${PROXY_HOST/%:[0-9]*}
export PROXY_PORT=${http_proxy/#http*:\/\/*:}

mkdir -p "${WORKSPACE}"

cat > "${WORKSPACE}/build.sh" << EOF_SCRIPT
#!/bin/bash

set -x

export http_proxy=${http_proxy}
export https_proxy=${http_proxy}
export ftp_proxy=${http_proxy}

cd "${WORKSPACE}"

# Go into the linux directory (the script will put us in a build subdir)
cd buildroot && make clean

# Build PPC64 defconfig
cat >> configs/powerpc64${ENDIANESS}_openpower_defconfig << EOF_BUILDROOT
BR2_powerpc64${ENDIANESS}=y
BR2_CCACHE=y
BR2_SYSTEM_BIN_SH_BASH=y
BR2_TARGET_GENERIC_GETTY_PORT="hvc0"
BR2_PACKAGE_BUSYBOX_SHOW_OTHERS=y
BR2_TARGET_ROOTFS_CPIO=y
BR2_TARGET_ROOTFS_CPIO_XZ=y
BR2_PACKAGE_IPMITOOL=y
# BR2_TARGET_ROOTFS_TAR is not set
EOF_BUILDROOT

# Build buildroot
export BR2_DL_DIR="${HOME}/buildroot_downloads"
make powerpc64${ENDIANESS}_openpower_defconfig
make

EOF_SCRIPT

chmod a+x "${WORKSPACE}/build.sh"

# Run the docker container, execute the build script we just built
docker run \
	--cap-add=sys_admin \
	--net=host \
	--rm=true \
	-e WORKSPACE="${WORKSPACE}" \
	--user="${USER}" \
	-w "${HOME}" \
	-v "${HOME}":"${HOME}" \
	-t initramfs-build/ubuntu \
	"${WORKSPACE}/build.sh"

# Timestamp for build
echo "Build completed, $(date)"

