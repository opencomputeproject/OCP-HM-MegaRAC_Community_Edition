#!/bin/bash
################################################################################
#
# Script used to create a Jenkins master that can run amd64 or ppc64le. It can
# be used to launch the Jenkins master as a Docker container locally or as a
# Kubernetes Deployment in a Kubernetes cluster.
#
################################################################################
#
# Script Variables:
#  build_scripts_dir  The path of the openbmc-build-scripts directory.
#                     Default: The directory containing this script
#  workspace          The directory that holds files used to build the Jenkins
#                     master master image and volumes used to deploy it.
#                     Default: "~/jenkins-build-${RANDOM}"
#
# Jenkins Dockerfile Variables:
#  agent_port         The port used as the Jenkins slave agent port.
#                     Default: "50000"
#  http_port          The port used as Jenkins UI port.
#                     Default: "8080"
#  img_name           The name given to the Docker image when it is built.
#                     Default: "openbmc/jenkins-master-${ARCH}:${JENKINS_VRSN}"
#  img_tag            The tag of the OpenJDK image used as the base image.
#                     Default: "/8-jdk"
#  j_gid              Jenkins group ID the container will use to run Jenkins.
#                     Default: "1000"
#  j_group            Group name tag the container will use to run Jenkins.
#                     Default: "jenkins"
#  j_home             Directory used as the Jenkins Home in the container.
#                     Default: "/var/jenkins_home"
#  j_uid              Jenkins user ID the container will use to run Jenkins.
#                     Default: "1000"
#  j_user             Username tag the container will use to run Jenkins.
#                     Default: "jenkins"
#  j_vrsn             The version of the Jenkins war file you wish to use.
#                     Default: "2.60.3"
#  tini_vrsn          The version of Tini to use in the Dockerfile, 0.16.1 is
#                     the first release with ppc64le release support.
#                     Default: "0.16.1"
#
# Deployment Variables:
#  cont_import_mnt    The directory on the container used to import extra files.
#                     Default: "/mnt/jenkins_import", ignored if above not set
#  home_mnt           The directory on the host used as the Jenkins home.
#                     Default: "${WORKSPACE}/jenkins_home"
#  host_import_mnt    The directory on the host used to import extra files.
#                     Default: "", import mount is ignored if not set
#  java_options       What will be passed as the environment variable for the
#                     JAVA_OPTS environment variable.
#                     Default: "-Xmx4096m"
#  jenkins_options    What will be passed as the environment variable for the
#                     JENKINS_OPTS environment variable.
#                     Default: "--prefix=/jenkins"
#  launch             docker|k8s
#                     Method in which the container will be launched. Either as
#                     a Docker container launched via Docker, or by using a
#                     helper script to launch into a Kubernetes cluster.
#                     Default: "docker"
#
################################################################################
set -xeo pipefail
ARCH=$(uname -m)

# Script Variables
build_scripts_dir=${build_scripts_dir:-"$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"}
workspace=${workspace:-${HOME}/jenkins-build-${RANDOM}}

# Jenkins Dockerfile Variables
agent_port=${agent_port:-50000}
http_port=${http_port:-8080}
img_name=${img_name:-openbmc/jenkins-master-${ARCH}:${j_vrsn}}
j_gid=${j_gid:-1000}
j_group=${j_group:-jenkins}
j_home=${j_home:-/var/jenkins_home}
j_uid=${j_uid:-1000}
j_user=${j_user:-jenkins}
j_vrsn=${j_vrsn:-2.60.3}
img_tag=${img_tag:-8-jdk}
tini_vrsn=${tini_vrsn:-0.16.1}

# Deployment Variables
cont_import_mnt=${cont_import_mnt:-/mnt/jenkins_import}
home_mnt=${home_mnt:-${workspace}/jenkins_home}
host_import_mnt=${host_import_mnt:-}
java_options=${java_options:-"-Xmx4096m"}
jenkins_options=${jenkins_options:-"--prefix=/jenkins"}
launch=${launch:-docker}

# Save the Jenkins.war URL to a variable and SHA if we care about verification
j_url=https://repo.jenkins-ci.org/public/org/jenkins-ci/main/jenkins-war/${j_vrsn}/jenkins-war-${j_vrsn}.war

# Make or Clean WORKSPACE
if [[ -d ${workspace} ]]; then
  rm -rf ${workspace}/Dockerfile \
         ${workspace}/docker-jenkins \
         ${workspace}/plugins.* \
         ${workspace}/install-plugins.sh \
         ${workspace}/jenkins.sh \
         ${workspace}/jenkins-support \
         ${workspace}/init.groovy
else
  mkdir -p ${workspace}
fi

# Determine the prefix of the Dockerfile's base image
case ${ARCH} in
  "ppc64le")
    docker_base="ppc64le/"
    tini_arch="ppc64el"
    ;;
  "x86_64")
    docker_base=""
    tini_arch="amd64"
    ;;
  *)
    echo "Unsupported system architecture(${ARCH}) found for docker image"
    exit 1
esac

# Move Into the WORKSPACE
cd ${workspace}

# Make the Dockerfile
################################################################################
cat >> Dockerfile << EOF
FROM ${docker_base}openjdk:${img_tag}

RUN apt-get update && apt-get install -y git curl

ENV JENKINS_HOME ${j_home}
ENV JENKINS_SLAVE_AGENT_PORT ${agent_port}

# Jenkins will default to run with user jenkins, uid = 1000
# If you bind mount a volume from the host or a data container,
# ensure you use the same uid
RUN groupadd -g ${j_gid} ${j_group} && \
    useradd -d ${j_home} -u ${j_uid} -g ${j_gid} -m -s /bin/bash ${j_user}

# Jenkins home directory is a volume, so configuration and build history
# can be persisted and survive image upgrades
VOLUME ${j_home}

# /usr/share/jenkins/ref/ contains all reference configuration we want
# to set on a fresh new installation. Use it to bundle additional plugins
# or config file with your custom jenkins Docker image.
RUN mkdir -p /usr/share/jenkins/ref/init.groovy.d

# Use tini as subreaper in Docker container to adopt zombie processes
RUN curl -fsSL https://github.com/krallin/tini/releases/download/v${tini_vrsn}/tini-static-${tini_arch} \
    -o /bin/tini && \
    chmod +x /bin/tini

COPY init.groovy /usr/share/jenkins/ref/init.groovy.d/tcp-slave-agent-port.groovy

# could use ADD but this one does not check Last-Modified header neither does it allow to control checksum
# see https://github.com/docker/docker/issues/8331
RUN curl -fsSL ${j_url} -o /usr/share/jenkins/jenkins.war

ENV JENKINS_UC https://updates.jenkins.io
ENV JENKINS_UC_EXPERIMENTAL=https://updates.jenkins.io/experimental
RUN chown -R ${j_user} ${j_home} /usr/share/jenkins/ref

# for main web interface:
EXPOSE ${http_port}

# will be used by attached slave agents:
EXPOSE ${agent_port}

ENV COPY_REFERENCE_FILE_LOG ${j_home}/copy_reference_file.log
USER ${j_user}

COPY jenkins-support /usr/local/bin/jenkins-support
COPY jenkins.sh /usr/local/bin/jenkins.sh
ENTRYPOINT ["/bin/tini", "--", "/usr/local/bin/jenkins.sh"]

# from a derived Dockerfile, can use RUN plugins.sh active.txt to setup /usr/share/jenkins/ref/plugins from a support bundle
COPY plugins.sh /usr/local/bin/plugins.sh
COPY install-plugins.sh /usr/local/bin/install-plugins.sh

# Install plugins.txt plugins
COPY plugins.txt /usr/share/jenkins/ref/plugins.txt
RUN /usr/local/bin/install-plugins.sh < /usr/share/jenkins/ref/plugins.txt
EOF
################################################################################

# Clone in the jenkinsci docker jenkins repo and copy some files into WORKSPACE
git clone https://github.com/jenkinsci/docker.git docker-jenkins
cp docker-jenkins/init.groovy .
cp docker-jenkins/jenkins-support .
cp docker-jenkins/jenkins.sh .
cp docker-jenkins/plugins.sh .
cp docker-jenkins/install-plugins.sh .

# Generate Plugins.txt, the plugins you want installed automatically go here
################################################################################
cat >> plugins.txt << EOF
kubernetes
EOF
################################################################################

# Build the image
docker build -t ${img_name} .

if [[ ${launch} == "docker" ]]; then

  # Ensure directories that will be mounted exist
  if [[ ! -z ${host_import_mnt} && ! -d ${host_import_mnt} ]]; then
      mkdir -p ${host_import_mnt}
  fi

  if [[ ! -d ${home_mnt} ]]; then
    mkdir -p ${home_mnt}
  fi

  # Ensure directories that will be mounted are owned by the jenkins user
  if [[ "$(id -u)" != 0 ]]; then
    echo "Not running as root:"
    echo "Checking if j_gid and j_uid are the owners of mounted directories"
    test_1=$(ls -nd ${home_mnt} | awk '{print $3 " " $4}')
    if [[ "${test_1}" != "${j_uid} ${j_gid}" ]]; then
      echo "Owner of ${home_mnt} is not the jenkins user"
      echo "${test_1} != ${j_uid} ${j_gid}"
      will_fail=1
    fi
    if [[ ! -z "${host_import_mnt}" ]]; then
      test_2=$(ls -nd ${host_import_mnt} | awk '{print $3 " " $4}' )
      if [[ "${test_2}" != "${j_uid} ${j_gid}" ]]; then
        echo "Owner of ${host_import_mnt} is not the jenkins user"
        echo "${test_2} != ${j_uid} ${j_gid}"
        will_fail=1
      fi
    fi
    if [[ "${will_fail}" == 1 ]]; then
      echo "Failing before attempting to launch container"
      echo "Try again as root or use correct uid/gid pairs"
      exit 1
    fi
  else
    if [[ ! -z ${host_import_mnt} ]]; then
      chown -R ${j_uid}:${j_gid} ${host_import_mnt}
    fi
    chown -R ${j_uid}:${j_gid} ${home_mnt}
  fi

  #If we don't have import mount don't add to docker command
  if [[ ! -z ${host_import_mnt} ]]; then
   import_vol_cmd="-v ${host_import_mnt}:${cont_import_mnt}"
  fi
  # Launch the jenkins image with Docker
  docker run -d \
    ${import_vol_cmd} \
    -v ${home_mnt}:${j_home} \
    -p ${http_port}:8080 \
    -p ${agent_port}:${agent_port} \
    --env JAVA_OPTS=\"${java_options}\" \
    --env JENKINS_OPTS=\"${jenkins_options}\" \
    ${img_name}

elif [[ ${launch} == "k8s" ]]; then
  # launch using the k8s template
  source ${build_scripts_dir}/kubernetes/kubernetes-launch.sh Build-Jenkins false false
fi
