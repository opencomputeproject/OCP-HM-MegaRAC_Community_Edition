#!/bin/bash

set -ex

Dockerfile=$(cat << EOF
FROM ubuntu:15.10
RUN DEBIAN_FRONTEND=noninteractive apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -yy \
	make gcc g++ libsystemd-dev libc6-dev pkg-config
RUN groupadd -g ${GROUPS} ${USER} && useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
USER ${USER}
ENV HOME ${HOME}
EOF
)

docker pull ubuntu:15.10
docker build -t openbmc/phosphor-event - <<< "${Dockerfile}"


docker run --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" openbmc/phosphor-event gcc --version
docker run --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" openbmc/phosphor-event make
docker run --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" openbmc/phosphor-event make check
