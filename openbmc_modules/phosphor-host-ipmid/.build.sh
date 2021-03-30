#!/bin/bash

Dockerfile=$(cat << EOF
FROM ubuntu:15.10
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get upgrade -yy
RUN DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -yy make g++ gcc libsystemd-dev libc6-dev pkg-config
RUN groupadd -g ${GROUPS} ${USER} && useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)

docker pull ubuntu:15.10
docker build -t temp - <<< "${Dockerfile}"

gcc --version

docker run --cap-add=sys_admin --net=host --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" -t temp make
