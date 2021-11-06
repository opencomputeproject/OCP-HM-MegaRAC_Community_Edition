#KBRANCH ?= "dev-5.8"
LINUX_VERSION ?= "5.4.85"

#SRCREV="7ee2d5b4d43403537df38d43c29f2a1bcb4dbce4"

require linux-aspeed.inc

python () {
    d.setVar('S', '${WORKDIR}/linux-5.4.85')
}

