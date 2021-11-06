SUMMARY = "pef and alert management application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI += "file://src \
	    file://include \
	    file://pef_configurations \
	    file://service_files \
	    file://CMakeLists.txt \
	   "
	    
SYSTEMD_SERVICE_${PN} += "pef-configuration.service \
                          pef-event-filtering.service \
                         "

DEPENDS = "sdbusplus \
           boost \
	   nlohmann-json \
   	   phosphor-logging \
	  " 

S = "${WORKDIR}"

inherit cmake systemd
FILES_${PN} += "/usr/share/pef-alert-manager/pef-alert-manager.json"

do_install() {
	install -d ${D}${bindir}
        install -m 0755 pef-configuration ${D}${bindir}
	install -m 0755 pef-event-filtering ${D}${bindir}

	install -d ${D}/${datadir}/pef-alert-manager
        install -m 0755 ${WORKDIR}/pef_configurations/pef-alert-manager.json ${D}/${datadir}/pef-alert-manager

	install -d ${D}${base_libdir}/systemd/system
        install -m 0644 ${S}/service_files/pef-configuration.service ${D}${base_libdir}/systemd/system
	install -m 0644 ${S}/service_files/pef-event-filtering.service ${D}${base_libdir}/systemd/system
}





