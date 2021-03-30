#!/bin/bash
###############################################################################
#
# This script is for initializing the Kubernetes environment needed to run all
# the kubernetes integrated scripts in Kubernetes.
# - Provisions the PV's and PVC's for:
#   * The Kubernetes JNLP Jenkins slave's shared workspace
#   * Shared state cache
#   * Openbmc/openbmc git reference repository
#   * Openbmc/qemu git reference repository
# - Create docker-registry secret for pulling from the internal repo
# - Create the config.json used to mount docker configuration to Kubernetes
#   Jenkins slaves that build and push docker images via shell scripts.
# Optionally:
# - Launch a Jenkins Master deployment into Kubernetes.
# - Provision the PV and PVC for the Jenkin Master home directory
#
# Instructions:
#  Suggested way to run is to create a separate script that will export all the
#  necessary variables and then source in this script. But editing this one
#  works as well.
#
###############################################################################
#
# Requirements:
#  - NFS server with directory to use as path for mount
#  - Access to an existing Kubernetes Cluster
#  - Kubectl installed and configured on machine running script
#
###############################################################################
#
# Variables used to initialize environment:
#  build_scripts_dir  The path for the openbmc-build-scripts directory.
#                     Default: The parent directory containing this script
#  email              The email that will be used to login to the regserver.
#                     Default: "email@place.holder", placeholder.
#  k8s_master         Set to True if you want to deploy a Jenkins Master into
#                     the Kubernetes deployment.
#                     Default: True
#  nfs_ip             IP address of the NFS server we will be using for mounting
#                     a Persistent Volume (PV) to. This should be replaced with
#                     an actual IP address of an NFS server.
#                     Default: "10.0.0.0", placeholder
#  ns                 Name of namespace the components will be deployed into.
#                     Default:"openbmc"
#  pass               The password that will be used to login to the regserver.
#                     Default: "password", placeholder
#  path_prefix        The prefix we will add to the nfspath of the directories
#                     we intend to mount. This is used to place all the
#                     different directories into the same parent folder on the
#                     NFS server.
#                     Default: "/san_mount/openbmc_k8s", placeholder
#  reclaim            The reclaim policy that will be used when creating the PV
#                     look at k8s docs for more info on this.
#                     Default: "Retain"
#  reg_server         The docker registry which will be used when pushing and
#                     pulling images. For internal use, it will be the internal
#                     registry created by ICP.
#                     Default: "master.icp:8500", placeholder
#  username           The username that will be used to login to the regserver.
#                     Default: "admin", placeholder
###############################################################################

# Variables used to initialize environment:
build_scripts_dir=${build_scripts_dir:-"$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/.."}
email=${email:-email\@place.holder}
k8s_master=${k8s_master:-True}
nfs_ip=${nfs_ip:-10.0.0.0}
ns=${ns:-openbmc}
pass=${pass:-password}
path_prefix=${path_prefix:-/san_mount/openbmc_k8s}
reclaim=${reclaim:-Retain}
reg_server=${reg_server:-master.icp:8500}
username=${username:-admin}

echo "Create the Jenkins Slave Workspace PVC"
name="jenkins-slave-space"
size="100Gi"
mode="ReadWriteMany"
nfs_path="${path_prefix}/jenkins-slave-space"
source ${build_scripts_dir}/kubernetes/storage-setup.sh

echo "Create the Shared State Cache PVC"
name="shared-state-cache"
size="100Gi"
mode="ReadWriteMany"
nfs_path="${path_prefix}/sstate-cache"
source ${build_scripts_dir}/kubernetes/storage-setup.sh

echo "Create the Openbmc Reference PVC"
name="openbmc-reference-repo"
size="1Gi"
mode="ReadWriteMany"
nfs_path="${path_prefix}/openbmc"
source ${build_scripts_dir}/kubernetes/storage-setup.sh

echo "Create the QEMU Reference PVC"
name="qemu-repo"
size="1Gi"
mode="ReadWriteMany"
nfs_path="${path_prefix}/qemu"
source ${build_scripts_dir}/kubernetes/storage-setup.sh

# Create the regkey secret for the internal docker registry
kubectl create secret docker-registry regkey -n $ns \
--docker-username=${username} \
--docker-password=${pass} \
--docker-email=${email} \
--docker-server=${reg_server}

# Create the docker config.json secret using the base64 encode of
# '${username}:${pass}'

base64up=$( echo -n "${username}:${pass}" | base64 )
cat >> config.json << EOF
{
  "auths": {
    "${regserver}": {
      "auth": "${base64up}"
    }
  }
}
EOF

chmod ugo+rw config.json
kubectl create secret generic docker-config -n $ns --from-file=./config.json
rm -f ./config.json

if [[ "${k8s_master}" ==  "True" ]]; then
  # Create the Jenkins Master Home PVC
  echo "Create the Jenkins Master Home PVC"
  name="jenkins-home"
  size="2Gi"
  mode="ReadWriteOnce"
  nfspath="${path_prefix}/jenkins-master-home"
  source ${build_scripts_dir}/kubernetes/storage-setup.sh

  # Launch the Jenkins Master
  launch="k8s"
  # Clean up variables before sourcing the build-jenkins.sh
  unset ns \
  nfsip \
  regserver \
  reclaim \
  path_prefix \
  username \
  pass email
  source ${build_scripts_dir}/build-jenkins.sh
fi
