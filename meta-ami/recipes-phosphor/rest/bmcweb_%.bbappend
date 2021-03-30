FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
DEPENDS += " gawk ipmi-oem libpeci peci-pcie "
RDEPENDS_${PN} += " gawk ipmi-oem libpeci peci-pcie "
SRC_URI_append += "file://0001-hostname-issue-in-Redfish-EIP-532870.patch \
		   file://0002-Getting-eth-speed-in-redfish.patch \
		   file://0003-removed-oem-related-in-redfish.patch \
		   file://0004-Added-SerialInterface-support-in-redfish.patch \
		   file://0005-Fixed-VM-stops-when-webui-timeout.patch \
		   file://0006-Added-Virtual-Media-Using-Redfish.patch \
		   file://0007-Added-Redfish-Allow-Headers.patch \
		   file://0008-fix-for-incomplete-localusers.patch \
		   file://0010-able-to-delete-login-user.patch \
		   file://0011-datetime-logentry-attribute.patch \
		   file://0012-Refresh-LdapConfigdata-when-dissable-Ldap.patch \
		   file://0013-syslog-redfish-file-directory-change.patch \
		   file://0014-Redish-V1.9-Update-OSP2.1.patch \
		   file://0015-added-interface-for-cpld-firmware.patch \
		   file://0016-Added-sensor_uri-under-chassis-collection.patch \
		   file://0017-Added-Redfish-LogService-Event-Type-Severity.patch \
		   file://0018-Added-NA-Attribute-sensor-status.patch \
		   file://0019-Added-Redfish-Sensor-RoundOff.patch \
		   file://0020-Added-Redfish-Session-Service.patch \
		   file://0021-HostIface-Redfish.patch \
                   file://0022-pam-maxtries-error-code.patch \
                   file://0023-clear-ipmi-sel-log.patch \
		   file://0024-Redfish-inventory.patch \
		   file://0025-Added-message-registaries.patch \
		   file://0026-DMTF-Validator-Fix.patch \
                   file://0027-remove-threshold-log-file.patch \
		  "

# Increase body limit size for FW
EXTRA_OECMAKE += "-DBMCWEB_HTTP_REQ_BODY_LIMIT_MB=65 -DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL=ON"
