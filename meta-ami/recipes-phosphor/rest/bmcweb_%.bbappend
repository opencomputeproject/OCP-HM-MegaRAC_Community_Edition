FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
DEPENDS += " gawk ipmi-oem "
RDEPENDS_${PN} += " gawk ipmi-oem "
SRC_URI_append += "file://0001-hostname-issue-in-Redfish-EIP-532870.patch \
		   file://0002-Getting-eth-speed-in-redfish.patch \
		   file://0003-removed-oem-related-in-redfish.patch \
		   file://0004-Added-SerialInterface-support-in-redfish.patch \
		   file://0005-Fixed-VM-stops-when-webui-timeout.patch \
		   file://0006-Added-Virtual-Media-Using-Redfish.patch \
		   file://0007-Added-Redfish-Allow-Headers.patch \
		   file://0008-Redfish-LogService-Event-Type-Severity.patch \
		   file://0009-Redfish-Sensor-RoundOff.patch \
		   file://0010-Redfish-Host-Iface.patch \
		   file://0010-able-to-delete-login-user.patch \
		   file://0011-Redish-V1.9-Update-OSP2.1-to-OSP2.2.patch \
		   file://0012-Added-Redfish-FRU-URI-Schema.patch \
		   file://0013-Added-VM-Redfish-support-through-NFS.patch \
		   file://0014-Added-message-registaries.patch \
		   file://0015-Multiple-session-VM-redirect-support.patch \
		   file://0016-Refresh-LdapConfigdata-when-dissable-Ldap.patch \
		   file://0017-Added-sensor_uri-under-chassis-collection.patch \
		   file://0018-Added-fru_URI-support-under-redfish-chassis_URI.patch \
		   file://0019-Set-websocket-idle_timeout-and-enable-keepalive-to-server-role.patch \
		   file://0020-support-rhel-large-size-image-redirection.patch \
		   file://0020-Enabled-tls-support.patch \
                   file://0021-passwordPolicy.patch \
		   file://0021-clear-sel-log-from-webui.patch \
		   file://0022-Retrive-back-Serial-interface-patch.patch \
		   file://0023-vlan-priority.patch \
		   file://0024-Readfish_circular_sel_read.patch \
		   file://0025-Crash-issue.patch \
		   file://0028-Support-large-size-iso-through-nbd-proxy.patch \
	           file://0029-Sensors-Thermal-Crash.patch \
		   file://0030-Event-Service-exit.patch \
		   file://0031-Fixed-Redfish-Service-Validator-Issues-OSP2.2.patch \
	           file://0032-redfish-sel-read-update.patch \
	           file://0033-Updated-Message-Registry-For-Processor-Sensors.patch \
		   file://0034-Added-URL-for-certificates-in-redfish.patch \
		   file://0035-ssl-issue-redfish.patch \
                   file://0036-passwordpolicy-complexity.patch \
		   file://0037-Idle_timeout_for_Vmedia.patch \
		   file://0038-persistent_BMC_Data_Json.patch \
		   file://0039-passpolicy-complexity-fix.patch \
		   file://0040-Fixed-KVM-Popup-Warning-Issue.patch \
		   file://0041-password-policy-fix.patch \
		   file://0042-serial-port-issue-fixed.patch \
		   file://0043-Added-new-interface-to-save-Vmedia-Credentials.patch \
		   file://0044-Fixed-VM-Multi-Session-Issue.patch \
		   file://0045-Fixed-Multi-User-Vmedia-Failing-Issue.patch \
                   file://0046-BMCWeb-crash-after-Redfish-Clear-sel-action.patch \
                   file://00047-Added-sleep-to-close-handler.patch \
	           file://0048-Validator-Cert-fix.patch \		
                  "

do_compile_prepend () {
	cp -r ${TEMP_DIR}/projdef.h .

}


# Increase body limit size for FW
#EXTRA_OECMAKE += "-DBMCWEB_HTTP_REQ_BODY_LIMIT_MB=65 -DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL=ON"
EXTRA_OEMESON += "-Dhttp-body-limit=65 -Dredfish-bmc-journal=enabled "
