Kubernetes YAML Templates for OpenBMC
=====================================

This directory holds the templates that are used by the [kubernetes-launch.sh](https://github.com/openbmc/openbmc-build-scripts/tree/master/kubernetes/kubernetes-launch.sh)
to launch objects into the Kubernetes API. Certain templates have different types such as job and
pod. To understand the difference between the different types of objects please refer to the
[Kubernetes Documentation](https://kubernetes.io/docs/concepts/).

The templates within this directory all have variables that the kubernetes-launch.sh script fills
in. Below is a listing of all the variables that each template uses and where its expected defaults
come from. However there are some variables that are not template specific, those will be addressed
first. Below, "initial script" refers to whatever script from the parent [openbmc-build-scripts](https://github.com/openbmc/openbmc-build-scripts)
repo was used and "launch script" refers to the kubernetes-launch.sh.

##Non-Template Specific Variables
- **ARCH**: The desired architecture for the machine the container is expected to run on. This is
  defined in the initial script. Since the initial script will build the container.
- **namespace**: The namespaces the container will be launched within Kubernetes, defined in the
  launch script.
- **WORKSPACE**: The workspace, this stems from our use of Jenkins, defined in the initial script.
- **HOME**: The home directory path of the user that ran the initial script.
- **imgname**: Name of the image used to launch the container, carried from the initial script and
  updated in the launch script so that it also includes the registry in the front.
- **BUILD\_ID**: This is a variable that is carried by our Jenkins integration. Can be ignored.
  Normally exported before the initial script.
- **imgplsec**: The image pull secret used to access the registry the images are stored in. This is
  defined in the launch script.
- **podname**: The name of the pod, as will be seen in the Kubernetes API. Defined in the launch
  script.

##OpenBMC-Build
- **target**: The build target for the OpenBMC-Build. Defined in the build-setup.sh script.
- **hclaim**: The "home" PVC, the external mount for the home directory. Defined in the launch
  script.
- **sclaim**: The "shared-state" PVC, the external mount for the shared state cache directory used
  to reduce the time it takes to do the build using cache. Defined in the launch script.
- **oclaim**: The "openbmc-cache" PVC, the external mount for the OpenBMC directory that is used for
  the build. This is done to allow for us to use this for testing were we use a specific checkout of
  a git repo. Defined in the launch script.
- **obmcdir**: The OpenBMC directory path that is used internally in the container to do the build.
  Defined in the build-setup.sh script.
- **sscdir**: The shared state cache directory used to speed up build times. Defined in the
  build-setup.sh script. This is where the container will mount the sclaim.
- **obmcext**: The OpenBMC directory path that is used to create the internal copy. This is the path
  to which the oclaim will be mounted to. Defined in the build-setup.sh script.

##QEMU-Build
- **hclaim**: The "home" PVC, the external mount for the home directory. Defined in the launch
  script.
- **qclaim**: The "qemu" PVC, external mount that holds the QEMU repository used to create the
  image. Defined in the launch script.
- **qemudir**: The qemu directory path to which the qclaim will be mounted to. Defined in the
  qemu-build.sh script.
