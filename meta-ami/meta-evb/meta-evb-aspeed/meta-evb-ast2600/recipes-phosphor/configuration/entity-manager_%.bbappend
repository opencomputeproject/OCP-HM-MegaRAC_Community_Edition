FILESEXTRAPATHS:append := ":${THISDIR}/${PN}"

SRC_URI:append = " \
    file://ast2600-evb.json \
    "

do_install:append(){

     # Remove unnecessary config files. EntityManager spends significant time parsing these.
     rm -f ${D}/usr/share/entity-manager/configurations/*.json
     install -m 0444 ${WORKDIR}/ast2600-evb.json ${D}/usr/share/entity-manager/configurations

}
