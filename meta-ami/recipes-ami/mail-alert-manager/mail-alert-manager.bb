SUMMARY = "mail alert management application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI += "file://src \
	    file://include \
	    file://service_files \
	    file://conf \
	    file://CMakeLists.txt \
	   "
	    
SYSTEMD_SERVICE_${PN} += "mail-alert-manager.service \
			 "
FILES_${PN} += "/usr/share/alert/smtp-config.json"

DEPENDS = "sdbusplus \
           boost \
	   nlohmann-json \
   	   phosphor-logging \
	   libesmtp \
	  " 

S = "${WORKDIR}"

inherit cmake systemd

do_install() {
	install -d ${D}${bindir}
	install -m 0755 mail-alert-manager ${D}${bindir}

	install -d ${D}/${datadir}/alert
	install -m 0755 ${S}/conf/smtp-config.json ${D}/${datadir}/alert

	install -d ${D}${base_libdir}/systemd/system
	install -m 0644 ${S}/service_files/mail-alert-manager.service ${D}${base_libdir}/systemd/system
}

#do_install() {
#	install -d ${D}${bindir}
#        install -m 0755 pef-alert-manager ${D}${bindir}
#
#	install -d ${D}${base_libdir}/systemd/system
#        install -m 0644 ${S}/pef-alert-manager.service ${D}${base_libdir}/systemd/system
#}




