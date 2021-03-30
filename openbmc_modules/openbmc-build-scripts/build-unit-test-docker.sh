#!/usr/bin/env bash
#
# Build the required docker image to run package unit tests
#
# Script Variables:
#   DOCKER_IMG_NAME:  <optional, the name of the docker image to generate>
#                     default is openbmc/ubuntu-unit-test
#   DISTRO:           <optional, the distro to build a docker image against>
#                     default is ubuntu:eoan
#   BRANCH:           <optional, branch to build from each of the openbmc/
#                     repositories>
#                     default is master, which will be used if input branch not
#                     provided or not found
#   UBUNTU_MIRROR:    <optional, the URL of a mirror of Ubuntu to override the
#                     default ones in /etc/apt/sources.list>
#                     default is empty, and no mirror is used.

set -xeuo pipefail

DOCKER_IMG_NAME=${DOCKER_IMG_NAME:-"openbmc/ubuntu-unit-test"}
DISTRO=${DISTRO:-"ubuntu:focal"}
BRANCH=${BRANCH:-"master"}
UBUNTU_MIRROR=${UBUNTU_MIRROR:-""}

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

# Setup temporary files
DEPCACHE_FILE=""
cleanup() {
  local status="$?"
  if [[ -n "$DEPCACHE_FILE" ]]; then
    rm -f "$DEPCACHE_FILE"
  fi
  trap - EXIT ERR
  exit "$status"
}
trap cleanup EXIT ERR INT TERM QUIT
DEPCACHE_FILE="$(mktemp)"

HEAD_PKGS=(
  openbmc/phosphor-objmgr
  openbmc/sdbusplus
  openbmc/sdeventplus
  openbmc/stdplus
  openbmc/gpioplus
  openbmc/phosphor-logging
  openbmc/phosphor-dbus-interfaces
  open-power/pdbg
  openbmc/pldm
)

# Generate a list of depcache entries
# We want to do this in parallel since the package list is growing
# and the network lookup is low overhead but decently high latency.
# This doesn't worry about producing a stable DEPCACHE_FILE, that is
# done by readers who need a stable ordering.
generate_depcache_entry() {
  local package="$1"

  local tip
  # Need to continue if branch not found, hence || true at end
  tip=$(git ls-remote --heads "https://github.com/${package}" |
        grep "refs/heads/$BRANCH" | awk '{ print $1 }' || true)

  # If specific branch is not found then try master
  if [[ ! -n "$tip" ]]; then
    tip=$(git ls-remote --heads "https://github.com/${package}" |
         grep "refs/heads/master" | awk '{ print $1 }')
  fi

  # Lock the file to avoid interlaced writes
  exec 3>> "$DEPCACHE_FILE"
  flock 3
  echo "$package:$tip" >&3
  exec 3>&-
}
for package in "${HEAD_PKGS[@]}"; do
  generate_depcache_entry "$package" &
done
wait

# A list of package versions we are building
# Start off by listing the stating versions of third-party sources
declare -A PKG_REV=(
  [boost]=1.73.0
  [cereal]=v1.2.2
  [catch2]=v2.12.2
  [CLI11]=v1.9.0
  [fmt]=6.2.1
  # Snapshot from 2020-01-03
  [function2]=3a0746bf5f601dfed05330aefcb6854354fce07d
  # Snapshot from 2020-02-13
  [googletest]=23b2a3b1cf803999fb38175f6e9e038a4495c8a5
  # TODO - Move back to released json once gcc10 fix is available
  # [json]=v3.7.3
  # Snapshot from 2020-05-19
  [json]=5cfa8a586ee1a656190491c1de20a82fb40fab5d
  # Snapshot from 2019-05-24
  [lcov]=75fbae1cfc5027f818a0bb865bf6f96fab3202da
  # dev-5.0 2019-05-03
  [linux-headers]=8bf6567e77f7aa68975b7c9c6d044bba690bf327
  # Snapshot from 2019-09-03
  [libvncserver]=1354f7f1bb6962dab209eddb9d6aac1f03408110
  [span-lite]=v0.7.0
  # version from meta-openembedded/meta-oe/recipes-support/libtinyxml2/libtinyxml2_5.0.1.bb
  [tinyxml2]=37bc3aca429f0164adf68c23444540b4a24b5778
  # version from meta-openembedded/meta-oe/recipes-devtools/valijson/valijson_git.bb
  [valijson]=c2f22fddf599d04dc33fcd7ed257c698a05345d9
  # version from meta-openembedded/meta-oe/recipes-devtools/nlohmann-fifo/nlohmann-fifo_git.bb
  [fifo_map]=0dfbf5dacbb15a32c43f912a7e66a54aae39d0f9
)

# Turn the depcache into a dictionary so we can reference the HEAD of each repo
for line in $(cat "$DEPCACHE_FILE"); do
  linearr=($(echo "$line" | tr ':' ' '))
  PKG_REV["${linearr[0]}"]="${linearr[1]}"
done

# Define common flags used for builds
PREFIX="/usr/local"
CONFIGURE_FLAGS=(
  "--prefix=${PREFIX}"
)
CMAKE_FLAGS=(
  "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
  "-DBUILD_SHARED_LIBS=ON"
  "-DCMAKE_INSTALL_PREFIX:PATH=${PREFIX}"
)
MESON_FLAGS=(
  "--wrap-mode=nodownload"
  "-Dprefix=${PREFIX}"
)

stagename()
{
  local cooked="$1"

  if ! echo "$cooked" | grep -q '/'
  then
    cooked=openbmc-"$cooked"
  fi
  echo "$cooked" | tr '/' '-'
}

# Build the commands needed to compose our final image
COPY_CMDS=""
# We must sort the packages, otherwise we might produce an unstable
# docker file and rebuild the image unnecessarily
for pkg in $(echo "${!PKG_REV[@]}" | tr ' ' '\n' | LC_COLLATE=C sort -s); do
  COPY_CMDS+="COPY --from=$(stagename ${pkg}) ${PREFIX} ${PREFIX}"$'\n'
  # Workaround for upstream docker bug and multiple COPY cmds
  # https://github.com/moby/moby/issues/37965
  COPY_CMDS+="RUN true"$'\n'
done

################################# docker img # #################################
# Create docker image that can run package unit tests
if [[ "${DISTRO}" == "ubuntu"* ]]; then

MIRROR=""
if [[ -n "${UBUNTU_MIRROR}" ]]; then
    MIRROR="RUN echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME) main restricted universe multiverse\" > /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-updates main restricted universe multiverse\" >> /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-security main restricted universe multiverse\" >> /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-proposed main restricted universe multiverse\" >> /etc/apt/sources.list && \
        echo \"deb ${UBUNTU_MIRROR} \$(. /etc/os-release && echo \$VERSION_CODENAME)-backports main restricted universe multiverse\" >> /etc/apt/sources.list"

fi

Dockerfile=$(cat << EOF
FROM ${DOCKER_BASE}${DISTRO} as openbmc-base

${MIRROR}

ENV DEBIAN_FRONTEND noninteractive

ENV PYTHONPATH "/usr/local/lib/python3.8/site-packages/"

# We need the keys to be imported for dbgsym repos
# New releases have a package, older ones fall back to manual fetching
# https://wiki.ubuntu.com/Debug%20Symbol%20Packages
RUN apt-get update && ( apt-get install ubuntu-dbgsym-keyring || ( apt-get install -yy dirmngr && \
    apt-key adv --keyserver keyserver.ubuntu.com --recv-keys F2EDC64DC5AEE1F6B9C621F0C8CAB6595FDFF622 ) )

# Parse the current repo list into a debug repo list
RUN sed -n '/^deb /s,^deb [^ ]* ,deb http://ddebs.ubuntu.com ,p' /etc/apt/sources.list >/etc/apt/sources.list.d/debug.list

# Remove non-existent debug repos
RUN sed -i '/-\(backports\|security\) /d' /etc/apt/sources.list.d/debug.list

RUN cat /etc/apt/sources.list.d/debug.list

RUN apt-get update && apt-get dist-upgrade -yy && apt-get install -yy \
    gcc-10 \
    g++-10 \
    libc6-dbg \
    libc6-dev \
    libtool \
    bison \
    flex \
    cmake \
    python3 \
    python3-dev\
    python3-yaml \
    python3-mako \
    python3-pip \
    python3-setuptools \
    python3-git \
    python3-socks \
    pkg-config \
    autoconf \
    autoconf-archive \
    libsystemd-dev \
    systemd \
    libssl-dev \
    libevdev-dev \
    libevdev2-dbgsym \
    libjpeg-dev \
    libpng-dev \
    ninja-build \
    sudo \
    curl \
    git \
    dbus \
    iputils-ping \
    clang-10 \
    clang-format-10 \
    clang-tidy-10 \
    clang-tools-10 \
    shellcheck \
    npm \
    iproute2 \
    libnl-3-dev \
    libnl-genl-3-dev \
    libconfig++-dev \
    libsnmp-dev \
    valgrind \
    valgrind-dbg \
    libpam0g-dev \
    xxd \
    libi2c-dev \
    wget \
    libldap2-dev \
    libprotobuf-dev \
    libperlio-gzip-perl \
    libjson-perl \
    protobuf-compiler \
    libgpiod-dev \
    device-tree-compiler \
    cppcheck \
    libpciaccess-dev \
    libmimetic-dev

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 1000 \
  --slave /usr/bin/g++ g++ /usr/bin/g++-10 \
  --slave /usr/bin/gcov gcov /usr/bin/gcov-10 \
  --slave /usr/bin/gcov-dump gcov-dump /usr/bin/gcov-dump-10 \
  --slave /usr/bin/gcov-tool gcov-tool /usr/bin/gcov-tool-10

RUN pip3 install inflection
RUN pip3 install pycodestyle
RUN pip3 install jsonschema
RUN pip3 install meson==0.54.3

FROM openbmc-base as openbmc-lcov
RUN curl -L https://github.com/linux-test-project/lcov/archive/${PKG_REV['lcov']}.tar.gz | tar -xz && \
cd lcov-* && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-function2
RUN mkdir ${PREFIX}/include/function2 && \
curl -L -o ${PREFIX}/include/function2/function2.hpp https://raw.githubusercontent.com/Naios/function2/${PKG_REV['function2']}/include/function2/function2.hpp

FROM openbmc-base as openbmc-googletest
RUN curl -L https://github.com/google/googletest/archive/${PKG_REV['googletest']}.tar.gz | tar -xz && \
cd googletest-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} -DTHREADS_PREFER_PTHREAD_FLAG=ON .. && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-catch2
RUN curl -L https://github.com/catchorg/Catch2/archive/${PKG_REV['catch2']}.tar.gz | tar -xz && \
cd Catch2-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} -DBUILD_TESTING=OFF -DCATCH_INSTALL_DOCS=OFF .. && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-cereal
RUN curl -L https://github.com/USCiLab/cereal/archive/${PKG_REV['cereal']}.tar.gz | tar -xz && \
cp -a cereal-*/include/cereal/ ${PREFIX}/include/

FROM openbmc-base as openbmc-CLI11
RUN curl -L https://github.com/CLIUtils/CLI11/archive/${PKG_REV['CLI11']}.tar.gz | tar -xz && \
cd CLI11-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} -DCLI11_BUILD_DOCS=OFF -DBUILD_TESTING=OFF -DCLI11_BUILD_EXAMPLES=OFF .. && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-fmt
RUN curl -L https://github.com/fmtlib/fmt/archive/${PKG_REV['fmt']}.tar.gz | tar -xz && \
cd fmt-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} -DFMT_DOC=OFF -DFMT_TEST=OFF .. && \
make -j$(nproc) && \
make install

# nlohmann json provides all of its function in a single header file
# TODO - Go back to using a release once gcc 10 fixes are in
# curl -L -o ${PREFIX}/include/nlohmann/json.hpp https://github.com/nlohmann/json/releases/download/${PKG_REV['json']}/json.hpp
FROM openbmc-base as openbmc-json
RUN mkdir ${PREFIX}/include/nlohmann/ && \
curl -L -o ${PREFIX}/include/nlohmann/json.hpp https://raw.githubusercontent.com/nlohmann/json/${PKG_REV['json']}/single_include/nlohmann/json.hpp && \
ln -s nlohmann/json.hpp ${PREFIX}/include/json.hpp

FROM openbmc-base as openbmc-fifo_map
RUN curl -L https://github.com/nlohmann/fifo_map/archive/${PKG_REV['fifo_map']}.tar.gz | tar -xz && \
cd fifo_map-*/src && cp fifo_map.hpp ${PREFIX}/include/

FROM openbmc-base as openbmc-span-lite
RUN curl -L https://github.com/martinmoene/span-lite/archive/${PKG_REV['span-lite']}.tar.gz | tar -xz && \
cd span-lite-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} -DSPAN_LITE_OPT_BUILD_TESTS=OFF .. && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-linux-headers
RUN curl -L https://github.com/openbmc/linux/archive/${PKG_REV['linux-headers']}.tar.gz | tar -xz && \
cd linux-* && \
make -j$(nproc) defconfig && \
make INSTALL_HDR_PATH=/usr/local headers_install

FROM openbmc-base as openbmc-boost
RUN curl -L https://dl.bintray.com/boostorg/release/${PKG_REV['boost']}/source/boost_$(echo "${PKG_REV['boost']}" | tr '.' '_').tar.bz2 | tar -xj && \
cd boost_*/ && \
./bootstrap.sh --prefix=${PREFIX} --with-libraries=context,coroutine && \
./b2 && ./b2 install --prefix=${PREFIX}

FROM openbmc-base as openbmc-tinyxml2
RUN curl -L https://github.com/leethomason/tinyxml2/archive/${PKG_REV['tinyxml2']}.tar.gz | tar -xz && \
cd tinyxml2-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} .. && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-valijson
RUN curl -L https://github.com/tristanpenman/valijson/archive/${PKG_REV['valijson']}.tar.gz | tar -xz && \
cd valijson-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} -DINSTALL_HEADERS=1 -DBUILD_TESTS=0 .. && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-libvncserver
RUN curl -L https://github.com/LibVNC/libvncserver/archive/${PKG_REV['libvncserver']}.tar.gz | tar -xz && \
cd libvncserver-* && \
mkdir build && \
cd build && \
cmake ${CMAKE_FLAGS[@]} .. && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-stdplus
COPY --from=openbmc-fmt ${PREFIX} ${PREFIX}
COPY --from=openbmc-span-lite ${PREFIX} ${PREFIX}
RUN curl -L https://github.com/openbmc/stdplus/archive/${PKG_REV['openbmc/stdplus']}.tar.gz | tar -xz && \
cd stdplus-* && \
meson build ${MESON_FLAGS[@]} -Dtests=disabled -Dexamples=false && \
ninja -C build && \
ninja -C build install

FROM openbmc-base as openbmc-sdbusplus
RUN curl -L https://github.com/openbmc/sdbusplus/archive/${PKG_REV['openbmc/sdbusplus']}.tar.gz | tar -xz && \
cd sdbusplus-* && \
cd tools && ./setup.py install --root=/ --prefix=${PREFIX} && \
cd .. && meson build ${MESON_FLAGS[@]} -Dtests=disabled -Dexamples=disabled && \
ninja -C build && \
ninja -C build install

FROM openbmc-base as openbmc-sdeventplus
COPY --from=openbmc-function2 ${PREFIX} ${PREFIX}
COPY --from=openbmc-stdplus ${PREFIX} ${PREFIX}
RUN curl -L https://github.com/openbmc/sdeventplus/archive/${PKG_REV['openbmc/sdeventplus']}.tar.gz | tar -xz && \
cd sdeventplus-* && \
meson build ${MESON_FLAGS[@]} -Dtests=disabled -Dexamples=false && \
ninja -C build && \
ninja -C build install

FROM openbmc-base as openbmc-gpioplus
COPY --from=openbmc-stdplus ${PREFIX} ${PREFIX}
RUN curl -L https://github.com/openbmc/gpioplus/archive/${PKG_REV['openbmc/gpioplus']}.tar.gz | tar -xz && \
cd gpioplus-* && \
meson build ${MESON_FLAGS[@]} -Dtests=disabled -Dexamples=false && \
ninja -C build && \
ninja -C build install

FROM openbmc-base as openbmc-phosphor-dbus-interfaces
COPY --from=openbmc-sdbusplus ${PREFIX} ${PREFIX}
RUN curl -L https://github.com/openbmc/phosphor-dbus-interfaces/archive/${PKG_REV['openbmc/phosphor-dbus-interfaces']}.tar.gz | tar -xz && \
cd phosphor-dbus-interfaces-* && \
./bootstrap.sh && \
./configure ${CONFIGURE_FLAGS[@]} --enable-openpower-dbus-interfaces --enable-ibm-dbus-interfaces && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-phosphor-logging
COPY --from=openbmc-cereal ${PREFIX} ${PREFIX}
COPY --from=openbmc-sdbusplus ${PREFIX} ${PREFIX}
COPY --from=openbmc-sdeventplus ${PREFIX} ${PREFIX}
COPY --from=openbmc-phosphor-dbus-interfaces ${PREFIX} ${PREFIX}
COPY --from=openbmc-fifo_map ${PREFIX} ${PREFIX}
RUN curl -L https://github.com/openbmc/phosphor-logging/archive/${PKG_REV['openbmc/phosphor-logging']}.tar.gz | tar -xz && \
cd phosphor-logging-* && \
./bootstrap.sh && \
./configure ${CONFIGURE_FLAGS[@]} --enable-metadata-processing YAML_DIR=${PREFIX}/share/phosphor-dbus-yaml/yaml && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-phosphor-objmgr
COPY --from=openbmc-boost ${PREFIX} ${PREFIX}
COPY --from=openbmc-sdbusplus ${PREFIX} ${PREFIX}
COPY --from=openbmc-tinyxml2 ${PREFIX} ${PREFIX}
COPY --from=openbmc-phosphor-logging ${PREFIX} ${PREFIX}
RUN curl -L https://github.com/openbmc/phosphor-objmgr/archive/${PKG_REV['openbmc/phosphor-objmgr']}.tar.gz | tar -xz && \
cd phosphor-objmgr-* && \
./bootstrap.sh && \
./configure ${CONFIGURE_FLAGS[@]} && \
make -j$(nproc) && \
make install

FROM openbmc-base as open-power-pdbg
RUN curl -L https://github.com/open-power/pdbg/archive/${PKG_REV['open-power/pdbg']}.tar.gz | tar -xz && \
cd pdbg-* && \
./bootstrap.sh && \
./configure ${CONFIGURE_FLAGS[@]} && \
make -j$(nproc) && \
make install

FROM openbmc-base as openbmc-pldm
COPY --from=openbmc-sdbusplus ${PREFIX} ${PREFIX}
COPY --from=openbmc-sdeventplus ${PREFIX} ${PREFIX}
COPY --from=openbmc-boost ${PREFIX} ${PREFIX}
COPY --from=openbmc-phosphor-dbus-interfaces ${PREFIX} ${PREFIX}
COPY --from=openbmc-phosphor-logging ${PREFIX} ${PREFIX}
COPY --from=openbmc-json ${PREFIX} ${PREFIX}
COPY --from=openbmc-CLI11 ${PREFIX} ${PREFIX}
RUN curl -L https://github.com/openbmc/pldm/archive/${PKG_REV['openbmc/pldm']}.tar.gz | tar -xz && \
cd pldm-* && \
meson build ${MESON_FLAGS[@]} -Dtests=disabled && \
ninja -C build && \
ninja -C build install

# Build the final output image
FROM openbmc-base
${COPY_CMDS}

# Some of our infrastructure still relies on the presence of this file
# even though it is no longer needed to rebuild the docker environment
# NOTE: The file is sorted to ensure the ordering is stable.
RUN echo '$(LC_COLLATE=C sort -s "$DEPCACHE_FILE" | tr '\n' ',')' > /tmp/depcache

# Final configuration for the workspace
RUN grep -q ${GROUPS} /etc/group || groupadd -g ${GROUPS} ${USER}
RUN mkdir -p "$(dirname "${HOME}")"
RUN grep -q ${UID} /etc/passwd || useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
RUN sed -i '1iDefaults umask=000' /etc/sudoers
RUN echo "${USER} ALL=(ALL) NOPASSWD: ALL" >>/etc/sudoers

RUN /bin/bash
EOF
)
fi
################################# docker img # #################################

http_proxy=${http_proxy:-}
proxy_args=""
if [[ -n "${http_proxy}" ]]; then
  proxy_args="--build-arg http_proxy=${http_proxy} --build-arg https_proxy=${http_proxy}"
fi

# Build above image
docker build ${proxy_args} --network=host -t ${DOCKER_IMG_NAME} - <<< "${Dockerfile}"
