FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "511369844523794fd2dd1655528b48fe38b8e1e5"

SRC_URI += " \
	   file://0001-BMC-ARP-Control.patch \
           file://0002_readall_for_sensor_instanceStart.patch \
           file://0003-Add-system-Interface-Privilege-Confdition-Check.patch \
           file://0004-fix-systemInfo-parameter-response.patch \
           file://0005-VLAN-Priority.patch \
           file://0006-AutType-ErrorMsg.patch \
           file://0008-Enabled-SetSelTime-ipmi-Command.patch \
           file://0010-Add-warm-reset-support.patch \
           file://phosphor-ipmi-host-ami.service \
	   file://0011-Add-Compact-SDR-Type2-Support.patch \
           file://0012-Get-Channel-Payload-Support-Detailed-Information-bel.patch \
	   file://0037-Systemd_Restart_Wrappper.patch \
           file://dcmi-getActive-command.patch \
           file://0013-Enable-Ipv6-static-address-and-disable-ipv6-dynamic-address.patch \  
           file://dcmi_getDCMICapabilities_info_oldHandler_to_newHandler.patch \ 
           file://0014-master-write-read-shows-wrong-response-for-invalid-d.patch \
           file://0038-alongwith-fix-in-ethernet-interface-enable-disable-ipv6-static-IP.patch \
	   file://0015-dcmi_old_to_new_handler_implementation.patch \
           file://0039-Added-Diag-and-safe-boot-mode-support.patch \
           file://0017-Return-proper-error-response-for-reserved-data-bytes.patch \
	   file://0040-Disable-DHCP-for-IPv6-when-ipv6-source-is-set-Static.patch \
	   file://0041-Fix-to-block-IPv6-Router-Static-Address-Configuration.patch \
           file://0019-Fix-for-get-set-dcmi-configuration-parameter-2-disco.patch \
           file://0018-Added-proper-error-responses-for-set-DCMI-configuara.patch \
           file://0042-Block-Manually_Set-Gateway-MAC-Address.patch \ 
           file://0043-Add-Prefix-Length-at-Neighbor.patch \
           file://0042-Fix-To-Block-IPv6-Index-Over-16.patch \
	   file://0042-Set-IPv4-Static-Address-When-Disable-DHCP.patch \
	   file://0044-Set-IP-address-source-to-unspecified-address-source.patch \
           file://0045-Support-16-Max-IPv6-Address.patch \
           file://0045-Implement-IPMI-Command-IPv4-IPv6-Enabled-Disabled.patch \
           file://0031-IPMI-command-for-Set-System-Info-Param0-should-throw.patch \
           file://0033-Set-watchdog-timer-Accepting-the-reserved-timer-acti.patch \
           "

do_install:append(){
  install -d ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/sensorhandler.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/selutility.hpp ${D}${includedir}/phosphor-ipmi-host
  install -m 0644 -D ${S}/phosphor-ipmi-warm-reset.target ${D}${systemd_system_unitdir}
  install -m 0644 -D ${WORKDIR}/phosphor-ipmi-host-ami.service ${D}${systemd_system_unitdir}/phosphor-ipmi-host.service
}

FILES:${PN} += "${systemd_system_unitdir}/*"

