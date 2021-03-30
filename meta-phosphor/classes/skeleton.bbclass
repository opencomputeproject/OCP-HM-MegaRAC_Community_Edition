inherit skeleton-rev

HOMEPAGE = "http://github.com/openbmc/skeleton"

SRC_URI += "${SKELETON_URI}"
S = "${WORKDIR}/skeleton/${SKELETON_DIR}"
