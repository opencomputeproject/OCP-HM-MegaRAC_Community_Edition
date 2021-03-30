#!/bin/bash

set -eou pipefail
set -x

util_ensure_available() {
  local bin=$1
  if ! which ${bin}
  then
    echo Please install ${bin}
    return 1
  fi
  return 0
}

jenkins_get_job_config() {
  local host="$1"
  local job="$2"
  local config="$(mktemp --suffix=.xml config.XXXXXX)"
  local url="https://${host}/job/${job}/config.xml"
  wget --output-document="${config}" "${url}"
  echo ${config}
}

jenkins_get_job_repos() {
  local config="$1"
  if [ -n "${REPO}" ]
  then
    echo "${REPO}"
    return 0
  fi
  # xmllint is rubbish, so we need sed(1) to separate the results
  xmllint --xpath '//com.sonyericsson.hudson.plugins.gerrit.trigger.hudsontrigger.data.GerritProject/pattern' "${config}" | sed -e 's/<pattern>//g' -e 's/<\/pattern>/\n/g'
}

: "${JENKINS_HOST:=openpower.xyz}"
: "${JENKINS_JOB:=openbmc-repository-ci}"

util_ensure_available wget
util_ensure_available xmllint
util_ensure_available git

CONFIG=
REPO=

while [[ $# -gt 0 ]]
do
  key="$1"
  case "${key}" in
    -c|--config)
      CONFIG="$2"
      shift 2
      ;;
    -r|--repo|--repository)
      REPO="$2"
      shift 2
      ;;
    -h|--help)
      echo USAGE: DISTRO=DOCKERBASE $0 --config config.xml
      echo
      echo DOCKERBASE is the Docker Hub tag of the base image against which to
      echo build and test the repositories described in config.xml. Individual
      echo repositories can be tested against DOCKERBASE with the --repository
      echo option \(in place of --config\).
      exit 0
      ;;
    *)
      (>&2 echo Unrecognised argument \'$1\')
      shift
      ;;
  esac
done

if [ -z "${CONFIG}" ]
then
  CONFIG="$(jenkins_get_job_config "${JENKINS_HOST}" "${JENKINS_JOB}")"
fi

export UNIT_TEST_PKG=
export WORKSPACE=

git_clone_repo() {
  local prj_package="$1"
  local package="$(basename "${prj_package}")"
  local workspace="$2"
  if [ -d "${prj_package}" ]
  then
    git clone "${prj_package}" "${workspace}"/"${package}"
    return
  fi
  git clone https://gerrit.openbmc-project.xyz/openbmc/"${package}" "${workspace}"/"${package}"
}

jenkins_get_job_repos "${CONFIG}" | while read GERRIT_PROJECT
do
  UNIT_TEST_PKG=$(basename ${GERRIT_PROJECT})
  WORKSPACE="$(mktemp -d --tmpdir openbmc-build-scripts.XXXXXX)"
  git clone . "${WORKSPACE}"/openbmc-build-scripts
  git_clone_repo "${GERRIT_PROJECT}" "${WORKSPACE}"
  ./run-unit-test-docker.sh
  rm -rf "${WORKSPACE}"
done
