#!/bin/bash

Dockerfile=$(cat << EOF
FROM ubuntu:15.10
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get upgrade -yy
RUN DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -yy make gcc libsystemd-dev libc6-dev pkg-config
RUN groupadd -g ${GROUPS} ${USER} && useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)

docker pull ubuntu:15.10
docker build -t temp - <<< "${Dockerfile}"

gcc --version

mkdir -p linux
wget https://raw.githubusercontent.com/openbmc/linux/dev-4.3/include/uapi/linux/bt-host.h -O linux/bt-host.h

docker run --cap-add=sys_admin --net=host --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" -t temp make KERNEL_HEADERS=$PWD
