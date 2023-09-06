# MegaRAC OneTree
- MegaRAC OneTree (OT) is AMI’s next generation BMC firmware solution following MegaRAC SP-X 13.
- Based on Linux Foundation OpenBMC stack
- Built on pervasive, open-source industry tools, architecture, and standards such as Yocto, BitBake, OpenEmbedded, D-bus etc.. 
- Enriched with added core feature sets for platform manageability
- Backed by AMI’s premium customer support

### 1) Prerequisite

See the [Yocto documentation](https://docs.yoctoproject.org/ref-manual/system-requirements.html#required-packages-for-the-build-host)
for the latest requirements

#### Ubuntu
```
$ sudo apt install git python3-distutils gcc g++ make file wget \
    gawk diffstat bzip2 cpio chrpath zstd lz4 bzip2
```

#### Fedora
```
$ sudo dnf install git python3 gcc g++ gawk which bzip2 chrpath cpio
hostname file diffutils diffstat lz4 wget zstd rpcgen patch
```

### 2) OT Core AST2600EVB Build Instruction
```
- meta-ami/github-gitlab-url.sh
- Add the other meta layer and features (optional)
- TEMPLATECONF=meta-ami/meta-evb/meta-evb-aspeed/meta-evb-ast2600/conf/templates/default . openbmc-env
- bitbake obmc-phosphor-image
```

### 3) OCP Layer AST2600EVB Build Instruction
```
- git clone -b OCP-fix https://git.ami.com/core/OSP/firmware/ocp.git
- Add the other meta layer and features (optional)
- TEMPLATECONF=meta-ami/meta-evb/meta-evb-aspeed/meta-evb-ast2600/conf/templates/default . openbmc-env
- bitbake obmc-phosphor-image
```


### Notes
- By default root user is disabled in the stack except AST2600EVB
- uncomment EXTRA_IMAGE_FEATURES += "debug-tweaks" in build/conf/local.conf to enable the root user access



