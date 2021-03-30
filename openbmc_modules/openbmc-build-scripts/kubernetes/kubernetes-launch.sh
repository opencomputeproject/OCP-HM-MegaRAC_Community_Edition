#!/bin/bash
###############################################################################
#
# Script used to assist in launching Kubernetes jobs/pods. Expects to be used
# as an supplemental script to the scripts that want to launch their containers
# on a Kubernetes cluster.
#
###############################################################################
#
# Requirements:
#  - Docker login credentials defined inside ~/.docker/config.json
#  - Kubectl installed and configured on machine running the script
#  - Access to a Kubernetes Cluster using v1.5.2 or newer
#  - NFS directories for OpenBMC repo cache, BitBake shared state cache, and
#    shared Jenkins home directory that holds workspaces.
#  - All NFS directories should have RWX permissions for user being used to run
#    the build-setup.sh script
#  - Persistent Volume and Claims created and mounted to NFS directories
#  - Image pull secret exists for image pulls in Kubernetes cluster namespace
#
###############################################################################
# Script Variables:
#  build_scripts_dir  The path for the openbmc-build-scripts directory.
#                     Default: The parent directory containing this script
#
# Kubernetes Variables:
#  img_pl_sec         The image pull secret used to access registry if needed
#                     Default: "regkey"
#  job_timeout         The amount of time in seconds that the build will wait for
#                     the job to be created in the api of the cluster.
#                     Default: "60"
#  namespace          The namespace to be used within the Kubernetes cluster
#                     Default: "openbmc"
#  pod_timeout        The amount of time in seconds that the build will wait for
#                     the pod to start running on the cluster.
#                     Default: "600"
#  registry           The registry to use to pull and push images
#                     Default: "master.cfc:8500/openbmc/""
#
# YAML File Variables (No Defaults):
#  img_name           The name the image that will be passed to the kubernetes
#                     api to build the containers. The image with the tag
#                     img_name will be built in the invoker script. This script
#                     will then tag it to include the registry in the name, push
#                     it, and update the img_name to be what was pushed to the
#                     registry. Users should not include the registry in the
#                     original img_name.
#  pod_name           The name of the pod, needed to trace down the logs.
#
# Deployment Option Variables (No Defaults):
#  invoker            Name of what this script is being called by or for, used
#                     to determine the template to use for YAML file.
#  launch             Used to determine the template used for the YAML file,
#                     normally carried in by sourcing this script in another
#                     script that has declared it.
#  log                If set to true the script will tail the container logs
#                     as part of the bash script.
#  purge              If set to true it will delete the created object once this
#                     script ends.
#  workaround         Used to enable the logging workaround, when set will
#                     launch a modified template that waits for a command. In
#                     most cases it will be waiting to have a script run via
#                     kubectl exec. Required when using a version of Kubernetes
#                     that has known issues that impact the retrieval of
#                     container logs when using kubectl. Defaulting to be true
#                     whenever logging is enabled until ICP upgrades their
#                     Kubernetes version to a version that doesn't need this.
#
###############################################################################

# Script Variables
build_scripts_dir=${build_scripts_dir:-"$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/.."}

# Kubernetes Variables
img_pl_sec=${img_pl_sec:-regkey}
job_timeout=${job_timeout:-60}
namespace=${namespace:-openbmc}
pod_timeout=${pod_timeout:-600}
registry=${registry:-master.cfc:8500/openbmc/}

# Deployment Option Variables:
invoker=${invoker:-${1}}
launch=${launch:-${4}}
log=${log:-${2}}
purge=${purge:-${3}}
workaround=${workaround:-${log}}

# Set the variables for the specific invoker to fill in the YAML template
# Other variables in the template not declared here are declared by invoker
case ${invoker} in
  Build-Jenkins)
    deploy_name=${deploy_name:-jenkins-master}
    pod_name=${pod_name:-jenkins-master-pod}
    new_img_name="${img_repo}jenkins-master-${ARCH}:${j_vrsn}"
    h_claim=${h_claim:-jenkins-home}
    cluster_ip=${cluster_ip:-10.0.0.175}
    http_nodeport=${http_nodeport:-32222}
    agent_nodeport=${agent_nodeport:-32223}
    ;;
  OpenBMC-build)
    w_claim=${w_claim:-jenkins-slave-space}
    s_claim=${s_claim:-shared-state-cache}
    o_claim=${o_claim:-openbmc-reference-repo}
    new_img_name=${new_img_name:-${registry}${distro}:${img_tag}-${ARCH}}
    pod_name=${pod_name:-openbmc${BUILD_ID}-${target}-builder}
    ;;
  QEMU-build)
    pod_name=${pod_name:-qemubuild${BUILD_ID}}
    w_claim=${w_claim:-jenkins-slave-space}
    q_claim=${q_claim:-qemu-repo}
    new_img_name="${registry}${img_name}"
    ;;
  QEMU-launch)
    deploy_name=${deploy_name:-qemu-launch-deployment}
    pod_name=${pod_name:-qemu-instance}
    replicas=${replicas:-5}
    w_claim=${w_claim:-jenkins-slave-space}
    jenkins_subpath=${jenkins_subpath:-Openbmc-Build/openbmc/build}
    new_img_name="${registry}qemu-instance"
    ;;
  XCAT-launch)
    ;;
  generic)
    ;;
  *)
    exit 1
    ;;
esac

# Tag the image created by the invoker with a name that includes the registry
docker tag ${img_name} ${new_img_name}
img_name=${new_img_name}

# Push the image that was built to the image repository
docker push ${img_name}

if [[ "$ARCH" == x86_64 ]]; then
  ARCH=amd64
fi

extras=""
if [[ "${workaround}" == "true" ]]; then
  extras+="-v2"
fi

yaml_file=$(eval "echo \"$(<${build_scripts_dir}/kubernetes/Templates/${invoker}-${launch}${extras}.yaml)\"")
kubectl create -f - <<< "${yaml_file}"

# If launch is a job we have to find the pod_name with identifiers
if [[ "${launch}" == "job" ]]; then
  while [ -z ${replace} ]
  do
    if [ ${job_timeout} -lt 0 ]; then
      kubectl delete -f - <<< "${yaml_file}"
      echo "Timeout occurred before job was present in the API"
      exit 1
    else
      sleep 1
      let job_timeout-=1
    fi
    replace=$(kubectl get pods -n ${namespace} | grep ${pod_name} | awk 'print $1')
  done
  pod_name=${replace}
fi


# Once pod is running track logs
if [[ "${log}" == true ]]; then
  # Wait for Pod to be running
  check_status="kubectl describe pod ${pod_name} -n ${namespace}"
  status=$( ${check_status} | grep Status: )
  while [ -z "$( echo ${status} | grep Running)" ]
  do
    if [ ${pod_timeout} -lt 0 ]; then
      kubectl delete -f - <<< "${yaml_file}"
      echo "Timeout occurred before pod was Running"
      exit 1
    else
      sleep 1
      let pod_timeout-=1
    fi
    status=$( ${check_status} | grep Status: )
  done
  # Tail the logs of the pod, if workaround enabled start executing build script instead.
  if [[ "${workaround}" == "true" ]]; then
    kubectl exec -it ${pod_name} -n ${namespace} ${WORKSPACE}/build.sh
  else
    kubectl logs -f ${pod_name} -n ${namespace}
  fi
fi

# Delete the object if purge is true
if [[ "${purge}" == true ]]; then
  kubectl delete -f - <<< "${yaml_file}"
fi
