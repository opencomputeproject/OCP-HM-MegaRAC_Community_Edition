FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append += "file://0001-sensor-reinitalization-after-cold-reset.patch"
SRC_URI_append += "file://0002-no-networkd-reset-if-IPSRC-same-only-DHCP-static-IPSRC-allowed.patch"
SRC_URI_append += "file://0003-watchdog.patch"
SRC_URI_append += "file://0004-GetMsgFlags.patch"
SRC_URI_append += "file://0005-check-duplicate-IP.patch"
SRC_URI_append += "file://0006-Update-chassis-power-off-handler-and-Fix-PowerCycle-state-issue.patch"
SRC_URI_append += "file://0006-Implement-System-Guid-and-Set-Channel-Security.patch"
SRC_URI_append += "file://0014-local-global-check-for-MAC-address.patch"
SRC_URI_append += "file://0015-dcmi-activate-power-limit-reserved-length-fix.patch"
SRC_URI_append += "file://0016-dhcp-false-after-IPSRC-to-static.patch"
SRC_URI_append += "file://0017-reqdata-length-vaildation-dcmi-cmd.patch"
SRC_URI_append += "file://0021-get-set-user-payload-for-non-created-users.patch"
SRC_URI_append += "file://0022-proper-error-msg-get-set-dcmi-configuration.patch"
SRC_URI_append += "file://0023-fixed-datalen-issue.patch"
SRC_URI_append += "file://0031-get-payload-access-fix.patch"
SRC_URI_append += "file://0032-user-payload-access-fix.patch"
SRC_URI_append += "file://0033-oem-payload-type-fix.patch"
SRC_URI_append += "file://0034-system-boot-options-extension.patch"
SRC_URI_append += "file://0035-warm-reset-command.patch"
SRC_URI_append += "file://0036-variant.patch"
SRC_URI_append += "file://0037-Systemd_Restart_Wrappper.patch"
SRC_URI_append += "file://0038-dcmi-supported-commands.patch"
SRC_URI_append += "file://0039-chassiscap.patch"
SRC_URI_append += "file://0040-removed-whitelisting-for-write-read.patch"
SRC_URI_append += "file://0041-fixed-cc-dcmi-cmds.patch"
SRC_URI_append += "file://0042-ipmi-channel-privilege-support.patch"
SRC_URI_append += "file://0043-Set-VLAN-ID-Interface-Type-fix.patch"
SRC_URI_append += "file://0044-VLAN-Priority-Interface-Priority.patch"
SRC_URI_append += "file://0045-sys-interface-privilagecondition-check.patch"
SRC_URI_append += "file://0046-GetAuthType-Fix.patch"
SRC_URI_append += "file://0047-allow-creating-admin-user-over-kcs.patch"
SRC_URI_append += "file://0048-dcmi-set-configuration-error-msg.patch"
SRC_URI_append += "file://0049-Added-dont-log-property.patch"
SRC_URI_append += "file://0050-get-vlan-priority-return-value-fix.patch"

SRC_URI_append += "file://0051-IPMI-IPv6andIPv4-Support-in-GetLan.patch"
SRC_URI_append += "file://0052-IPMI-55-IPv6-Status-in-GetLan.patch"
SRC_URI_append += "file://0053-ipmi-param-56-IPV6-Static-Addres-Range-check-in-setLan.patch"
SRC_URI_append += "file://0054-ipmi-parameter51-IPv6andIPv4-Addressing.patch"
SRC_URI_append += "file://0055-set-get-DefaultGateway-each-interface.patch"
SRC_URI_append += "file://0056-proper-error-code-chassis-control.patch"
SRC_URI_append += "file://0056-set-ActiveDHCP-reset-issue.patch"
SRC_URI_append += "file://0057-fix_invalid_error_code_in_applyPowerLimit.patch"
SRC_URI_append += "file://0058-fix_err_code_in_GetPayloadVersionCmd.patch"
SRC_URI_append += "file://0059-fixedErrorCodeInGetSysInfoCmd.patch"
SRC_URI_append += "file://0060-fixIPMIWriteReadMaxLen.patch"
SRC_URI_append += "file://0061-fixed-get-systeminfo-issue.patch"
SRC_URI_append += "file://0062-setting-getDeviceGuid.patch"
SRC_URI_append += "file://0063-dcmi-get-temp-reading-instance-start.patch"
SRC_URI_append += "file://0064-Fixed-get-system-info.patch"
SRC_URI_append += "file://0065-dcmi-invalid-cmd-not-setting-parameters.patch"
SRC_URI_append += "file://0066-BMC-ARP-Control.patch"
SRC_URI_append += "file://0067-NCSI-VLAN-Implementation.patch"
SRC_URI_append += "file://0068-IPv6-Bugfix.patch"
SRC_URI_append += "file://0069-getUserPayloadAccess-getting-unknown-fix.patch"
SRC_URI_append += "file://0069-set-IPSrc-ret-err.patch"
SRC_URI_append += "file://0070-dcmi-param-2-SEL-attribute.patch"
SRC_URI_append += "file://0071-sensor-reinitalization-after-cold-reset.patch"
SRC_URI_append += "file://0072-FixedProperErrCodeInSetSysInfoCmd.patch"
SRC_URI_append += "file://0073-return-D6-for-unsupported-cmd.patch"
SRC_URI_append += "file://0074-fix-improper-responses-by-system-boot-options-system.patch"
SRC_URI_append += "file://0075-IPv6-Statesandfixes.patch"
SRC_URI_append += "file://0077-ipv4-address-source-fix.patch"
SRC_URI_append += "file://0076-VLAN-bugfixes.patch"

FILES_${PN} += "${systemd_system_unitdir}/phosphor-ipmi-warm-reset.target"

do_install_append(){
  install -d ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/user_channel/cipher_mgmt.hpp ${D}${includedir}/user_channel
  install -m 0644 -D ${S}/sensorhandler.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/selutility.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/phosphor-ipmi-host-ami.service ${D}${systemd_system_unitdir}/phosphor-ipmi-host.service
  install -m 0644 -D ${S}/phosphor-ipmi-warm-reset.target ${D}${systemd_system_unitdir}/phosphor-ipmi-warm-reset.target
}
