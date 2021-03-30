#MRW_API_SRC_URI ?= "git://github.com/open-power/serverwiz.git"
#MRW_API_SRCREV ?= "60c8e10cbb11768cd1ba394b35cb1d6627efec42"

FILESPATH =. "${TOPDIR}/../openbmc_modules:"
MRW_TOOLS_SRC_URI ?= "file://phosphor-mrw-tools"
MRW_TOOLS_SRCREV ?= "${AUTOREV}"
S = "${WORKDIR}/phosphor-mrw-tools"

