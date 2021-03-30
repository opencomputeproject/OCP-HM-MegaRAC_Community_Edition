#!/bin/bash
###############################################################################
#
# This build script is for running the OpenBMC builds as Docker containers.
#
###############################################################################
#
# Script Variables:
#  build_scripts_dir  The path of the openbmc-build-scripts directory.
#                     Default: The directory containing this script
#  http_proxy         The HTTP address of the proxy server to connect to.
#                     Default: "", proxy is not setup if this is not set
#  WORKSPACE          Path of the workspace directory where some intermediate
#                     files and the images will be saved to.
#                     Default: "~/{RandomNumber}"
#  num_cpu            Number of cpu's to give bitbake, default is total amount
#                     in system
#  UBUNTU_MIRROR:     <optional, the URL of a mirror of Ubuntu to override the
#                     default ones in /etc/apt/sources.list>
#                     default is empty, and no mirror is used.
#
# Docker Image Build Variables:
#  BITBAKE_OPTS       Set to "-c populate_sdk" or whatever other BitBake options
#                     you'd like to pass into the build.
#                     Default: "", no options set
#  build_dir          Path where the actual BitBake build occurs inside the
#                     container, path cannot be located on network storage.
#                     Default: "/tmp/openbmc"
#  distro             The distro used as the base image for the build image:
#                     fedora|ubuntu
#                     Default: "ubuntu"
#  img_name           The name given to the target build's docker image.
#                     Default: "openbmc/${distro}:${imgtag}-${target}-${ARCH}"
#  img_tag            The base docker image distro tag:
#                     ubuntu: latest|16.04|14.04|trusty|xenial
#                     fedora: 23|24|25
#                     Default: "latest"
#  target             The target we aim to build:
#                     evb-ast2500|palmetto|qemu|qemux86-64
#                     romulus|s2600wf|witherspoon|zaius|tiogapass|gsj|mihawk
#                     witherspoon-tacoma|rainier
#                     Default: "qemu"
#  no_tar             Set to true if you do not want the debug tar built
#                     Default: "false"
#  nice_priority      Set nice priotity for bitbake command.
#                     Nice:
#                       Run with an adjusted niceness, which affects process
#                       scheduling. Nice values range from -20 (most favorable
#                       to the process) to 19 (least favorable to the process).
#                     Default: "", nice is not used if nice_priority is not set
#
# Deployment Variables:
#  obmc_dir           Path of the OpenBMC repo directory used as a reference
#                     for the build inside the container.
#                     Default: "${WORKSPACE}/openbmc"
#  xtrct_small_copy_dir
#                     Directory within build_dir that should be copied to
#                     xtrct_path. The directory and all parents up to, but not
#                     including, build_dir will be copied. For example, if
#                     build_dir is set to "/tmp/openbmc" and this is set to
#                     "build/tmp", the directory at xtrct_path will have the
#                     following directory structure:
#                     xtrct_path
#                      | - build
#                        | - tmp
#                          ...
#                     Can also be set to the empty string to copy the entire
#                     contents of build_dir to xtrct_path.
#                     Default: "deploy/images".
#
###############################################################################
# Trace bash processing. Set -e so when a step fails, we fail the build
set -xeo pipefail

# Script Variables:
build_scripts_dir=${build_scripts_dir:-"$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"}
http_proxy=${http_proxy:-}
WORKSPACE=${WORKSPACE:-${HOME}/${RANDOM}${RANDOM}}
num_cpu=${num_cpu:-$(nproc)}
UBUNTU_MIRROR=${UBUNTU_MIRROR:-""}

# Docker Image Build Variables:
build_dir=${build_dir:-/tmp/openbmc}
distro=${distro:-ubuntu}
img_tag=${img_tag:-latest}
target=${target:-qemu}
no_tar=${no_tar:-false}
nice_priority=${nice_priority:-}

# Deployment variables
obmc_dir=${obmc_dir:-${WORKSPACE}/openbmc}
ssc_dir=${HOME}
xtrct_small_copy_dir=${xtrct_small_copy_dir:-deploy/images}
xtrct_path="${obmc_dir}/build/tmp"
xtrct_copy_timeout="300"

bitbake_target="obmc-phosphor-image"
PROXY=""

MIRROR=""
if [[ -n "${UBUNTU_MIRROR}" ]]; then
    MIRROR="RUN echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME) main restricted universe multiverse\" > /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-updates main restricted universe multiverse\" >> /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-security main restricted universe multiverse\" >> /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-proposed main restricted universe multiverse\" >> /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-backports main restricted universe multiverse\" >> /etc/apt/sources.list"
fi

# Determine the architecture
ARCH=$(uname -m)

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

# Timestamp for job
echo "Build started, $(date)"

# If the obmc_dir directory doesn't exist clone it in
if [ ! -d ${obmc_dir} ]; then
  echo "Clone in openbmc master to ${obmc_dir}"
  git clone https://github.com/openbmc/openbmc ${obmc_dir}
fi

# Make and chown the xtrct_path directory to avoid permission errors
if [ ! -d ${xtrct_path} ]; then
  mkdir -p ${xtrct_path}
fi
chown ${UID}:${GROUPS} ${xtrct_path}

# Work out what build target we should be running and set BitBake command
MACHINE=""
case ${target} in
  palmetto)
    LAYER_DIR="meta-ibm/meta-palmetto"
    MACHINE="palmetto"
    DISTRO="openbmc-openpower"
    ;;
  swift)
    LAYER_DIR="meta-ibm"
    MACHINE="swift"
    DISTRO="openbmc-witherspoon"
    ;;
  mihawk)
    LAYER_DIR="meta-ibm"
    MACHINE="mihawk"
    DISTRO="openbmc-witherspoon"
    ;;
  witherspoon)
    LAYER_DIR="meta-ibm"
    MACHINE="witherspoon"
    DISTRO="openbmc-witherspoon"
    ;;
  witherspoon-128)
    LAYER_DIR="meta-ibm"
    MACHINE="witherspoon-128"
    DISTRO="openbmc-witherspoon"
    ;;
  witherspoon-tacoma)
    LAYER_DIR="meta-ibm"
    MACHINE="witherspoon-tacoma"
    DISTRO="openbmc-openpower"
    ;;
  rainier)
    LAYER_DIR="meta-ibm"
    MACHINE="rainier"
    DISTRO="openbmc-openpower"
    ;;
  evb-ast2500)
    LAYER_DIR="meta-evb/meta-evb-aspeed/meta-evb-ast2500"
    MACHINE="evb-ast2500"
    DISTRO="openbmc-phosphor"
    ;;
  s2600wf)
    LAYER_DIR="meta-intel/meta-s2600wf"
    MACHINE="s2600wf"
    DISTRO="openbmc-phosphor"
    ;;
  zaius)
    LAYER_DIR="meta-ingrasys/meta-zaius"
    MACHINE="zaius"
    DISTRO="openbmc-openpower"
    ;;
  romulus)
    LAYER_DIR="meta-ibm/meta-romulus"
    MACHINE="romulus"
    DISTRO="openbmc-openpower"
    ;;
  tiogapass)
    LAYER_DIR="meta-facebook/meta-tiogapass"
    MACHINE="tiogapass"
    DISTRO="openbmc-phosphor"
    ;;
  gsj)
    LAYER_DIR="meta-quanta/meta-gsj"
    MACHINE="gsj"
    # Use default DISTRO from layer
    ;;
  *)
    exit 1
    ;;
esac

BITBAKE_CMD="TEMPLATECONF=${LAYER_DIR}/conf source oe-init-build-env"

# Configure Docker build
if [[ "${distro}" == fedora ]];then

  if [[ -n "${http_proxy}" ]]; then
    PROXY="RUN echo \"proxy=${http_proxy}\" >> /etc/dnf/dnf.conf"
  fi

  Dockerfile=$(cat << EOF
  FROM ${DOCKER_BASE}${distro}:${img_tag}

  ${PROXY}

  # Set the locale
  RUN locale-gen en_US.UTF-8
  ENV LANG en_US.UTF-8
  ENV LANGUAGE en_US:en
  ENV LC_ALL en_US.UTF-8

  RUN dnf --refresh install -y \
      bzip2 \
      chrpath \
      cpio \
      diffstat \
      findutils \
      gcc \
      gcc-c++ \
      git \
      make \
      patch \
      perl-bignum \
      perl-Data-Dumper \
      perl-Thread-Queue \
      python-devel \
      python3-devel \
      SDL-devel \
      socat \
      subversion \
      tar \
      texinfo \
      wget \
      which \
      iputils-ping

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
  FROM ${DOCKER_BASE}${distro}:${img_tag}

  ${PROXY}
  ${MIRROR}

  ENV DEBIAN_FRONTEND noninteractive

  RUN apt-get update && apt-get install -yy \
      build-essential \
      chrpath \
      debianutils \
      diffstat \
      gawk \
      git \
      libdata-dumper-simple-perl \
      libsdl1.2-dev \
      libthread-queue-any-perl \
      locales \
      python \
      python3 \
      socat \
      subversion \
      texinfo \
      cpio \
      wget \
      iputils-ping

  # Set the locale
  RUN locale-gen en_US.UTF-8
  ENV LANG en_US.UTF-8
  ENV LANGUAGE en_US:en
  ENV LC_ALL en_US.UTF-8

  RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
  RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}

  USER ${USER}
  ENV HOME ${HOME}
  RUN /bin/bash
EOF
)
fi

# Create the Docker run script
export PROXY_HOST=${http_proxy/#http*:\/\/}
export PROXY_HOST=${PROXY_HOST/%:[0-9]*}
export PROXY_PORT=${http_proxy/#http*:\/\/*:}

mkdir -p ${WORKSPACE}

# Determine command for bitbake image build
if [ $no_tar = "false" ]; then
    bitbake_target="${bitbake_target} obmc-phosphor-debug-tarball"
fi

cat > "${WORKSPACE}"/build.sh << EOF_SCRIPT
#!/bin/bash

set -xeo pipefail

# Go into the OpenBMC directory, the build will handle changing directories
cd ${obmc_dir}

# Set up proxies
export ftp_proxy=${http_proxy}
export http_proxy=${http_proxy}
export https_proxy=${http_proxy}

mkdir -p ${WORKSPACE}/bin

# Configure proxies for BitBake
if [[ -n "${http_proxy}" ]]; then

  cat > ${WORKSPACE}/bin/git-proxy << \EOF_GIT
  #!/bin/bash
  # \$1 = hostname, \$2 = port
  PROXY=${PROXY_HOST}
  PROXY_PORT=${PROXY_PORT}
  exec socat STDIO PROXY:\${PROXY}:\${1}:\${2},proxyport=\${PROXY_PORT}
EOF_GIT

  chmod a+x ${WORKSPACE}/bin/git-proxy
  export PATH=${WORKSPACE}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:${PATH}

  lock=${HOME}/build-setup.lock
  flock \${lock} git config --global core.gitProxy ${WORKSPACE}/bin/git-proxy
  flock \${lock} git config --global http.proxy ${http_proxy}

  flock \${lock} mkdir -p ~/.subversion
  flock \${lock} cat > ~/.subversion/servers << EOF_SVN
  [global]
  http-proxy-host = ${PROXY_HOST}
  http-proxy-port = ${PROXY_PORT}
EOF_SVN

  flock \${lock} cat > ~/.wgetrc << EOF_WGETRC
  https_proxy = ${http_proxy}
  http_proxy = ${http_proxy}
  use_proxy = on
EOF_WGETRC

  flock \${lock} cat > ~/.curlrc << EOF_CURLRC
  proxy = ${PROXY_HOST}:${PROXY_PORT}
EOF_CURLRC
fi

# Source our build env
${BITBAKE_CMD}

if [[ -z "${MACHINE}" ]]; then
  echo "MACHINE is not configured for ${target}"
  exit 1
fi

export MACHINE="${MACHINE}"
if [[ -z "${DISTRO}" ]]; then
  echo "DISTRO is not configured for ${target} so will use default"
  unset DISTRO
else
  export DISTRO="${DISTRO}"
fi

# Custom BitBake config settings
cat >> conf/local.conf << EOF_CONF
BB_NUMBER_THREADS = "$(nproc)"
PARALLEL_MAKE = "-j$(nproc)"
INHERIT += "rm_work"
BB_GENERATE_MIRROR_TARBALLS = "1"
DL_DIR="${ssc_dir}/bitbake_downloads"
SSTATE_DIR="${ssc_dir}/bitbake_sharedstatecache"
USER_CLASSES += "buildstats"
INHERIT_remove = "uninative"
TMPDIR="${build_dir}"
EOF_CONF

# Kick off a build
if [[ -n "${nice_priority}" ]]; then
    nice -${nice_priority} bitbake ${BITBAKE_OPTS} ${bitbake_target}
else
    bitbake ${BITBAKE_OPTS} ${bitbake_target}
fi

# Copy internal build directory into xtrct_path directory
if [[ ${xtrct_small_copy_dir} ]]; then
  mkdir -p ${xtrct_path}/${xtrct_small_copy_dir}
  timeout ${xtrct_copy_timeout} cp -r ${build_dir}/${xtrct_small_copy_dir}/* ${xtrct_path}/${xtrct_small_copy_dir}
else
  timeout ${xtrct_copy_timeout} cp -r ${build_dir}/* ${xtrct_path}
fi

if [[ 0 -ne $? ]]; then
  echo "Received a non-zero exit code from timeout"
  exit 1
fi

EOF_SCRIPT

chmod a+x ${WORKSPACE}/build.sh

# Give the Docker image a name based on the distro,tag,arch,and target
img_name=${img_name:-openbmc/${distro}:${img_tag}-${target}-${ARCH}}

# Build the Docker image
docker build -t ${img_name} - <<< "${Dockerfile}"

# If obmc_dir or ssc_dir are ${HOME} or a subdirectory they will not be mounted
mount_obmc_dir="-v ""${obmc_dir}"":""${obmc_dir}"" "
mount_ssc_dir="-v ""${ssc_dir}"":""${ssc_dir}"" "
mount_workspace_dir="-v ""${WORKSPACE}"":""${WORKSPACE}"" "
if [[ "${obmc_dir}" = "${HOME}/"* || "${obmc_dir}" = "${HOME}" ]];then
mount_obmc_dir=""
fi
if [[ "${ssc_dir}" = "${HOME}/"* || "${ssc_dir}" = "${HOME}" ]];then
mount_ssc_dir=""
fi
if [[ "${WORKSPACE}" = "${HOME}/"* || "${WORKSPACE}" = "${HOME}" ]];then
mount_workspace_dir=""
fi

# Run the Docker container, execute the build.sh script
docker run \
--cap-add=sys_admin \
--cap-add=sys_nice \
--net=host \
--rm=true \
-e WORKSPACE=${WORKSPACE} \
-w "${HOME}" \
-v "${HOME}":"${HOME}" \
${mount_obmc_dir} \
${mount_ssc_dir} \
${mount_workspace_dir} \
--cpus="$num_cpu" \
-t ${img_name} \
${WORKSPACE}/build.sh

# To maintain function of resources that used an older path, add a link
ln -sf ${xtrct_path}/deploy ${WORKSPACE}/deploy

# Timestamp for build
echo "Build completed, $(date)"
