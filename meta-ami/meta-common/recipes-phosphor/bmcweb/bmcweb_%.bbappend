FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON += "-Dredfish-dump-log=enabled"
EXTRA_OEMESON += "-Dredfish-new-powersubsystem-thermalsubsystem=enabled"
EXTRA_OEMESON += "-Dredfish-provisioning-feature=enabled"

# add "redfish-hostiface" group
GROUPADD_PARAM:${PN}:append = ";redfish-hostiface"

#SRCREV = "188cb6294105a045a445619415d01843de8c3732"

#SRC_URI:append	= " file://0006-enabled-redfish-dump-log.patch "
#SRC_URI:append	= " file://0014-Add-Download-BMCDump-Support-in-Debug-Collector.patch "

SRC_URI:append = "file://0001-managers-add-factory-restore.patch \
	    file://0002-virtual-media-nfs-support.patch \
	    file://0004-Added_Sevice_Conf_for_KVM_VM_SSLSOL.patch \
	    file://0005-added-IPv6StaticDefaultGateways-property.patch \
	    file://0007-Restricted-root-user-privilage.patch \
	    file://0008-enhanced-passwordpolicy.patch \
            file://0009-Post-Chassis.Reset-ChassisId-validation.patch \
            file://0010-Time-zone-configuration-support.patch \
            file://0011-Add-Chassis-Sensors-Collection.patch \
	    file://0015-added-OEM-led-indicator-amber-green-susack-status.patch \
	    file://0017-Integrated-NVME-Interface.patch \
	    file://0018-Add-Redfish-Logs-for-Discrete-Sensors.patch \
            file://0019-IndicatorLED-Depreacted.patch \
            file://0021-Integrated-NIC-Interface-in-Redfish.patch \
	    file://0022-Fixed-the-Enable-Disable-outband-IPMI-issue.patch \
	    file://0023-Fix-for-unable-to-delete-newly-created-ipv4-static.patch \
            file://0024-Time-Zone-Configuration-issue-fix.patch \ 
	    file://0025-Add-Diag-and-Safe-Mode-Support.patch \
	    file://0027-Fix-KVM-disconnect-issue.patch \
	    file://0029-Fix-getting-empty-IP-address.patch \
	    file://0030-Rolemapping-Mismatch-Fix.patch \
	    file://0030-FIXED-PATCH-operation-in-EthernetInterface.patch \
	    file://0031-Integrated-RAID-HBA-Interface.patch \
	    file://0032-Fixes-ethernetInternet-DHCP-property-patch-support.patch \
	    file://0033-Fixed-the-Attribute-MaxConcurrentSession-type-to-int.patch \
            file://0035-MaintenanceWindow-OperationApplyTime-Support.patch \
	    file://0036-Add-Locked-status-to-login-API-on-User-locked.patch \
	    file://0037-Fix-for-500-internal-error-in-ethernet-IPV6-patch-op.patch \
            file://0039-Unable-to-set-static-IPv4-IPv6-Address-in-WEBUI.patch \
	    file://0040-Fix-for-status-code-return-under-Chassis-URI.patch \
            file://0044-Restrict-the-patch-of-IPv4-from-DHCP-to-Static-and-v.patch \ 
	    file://0045-Fixes-LED-Button-display-issue-in-Overview-Page.patch \
	    file://0049-Fix-for-DateTimeLocalOffset-return-code-status.patch \
	    file://0052-Fixed-Apache-Benchmark-tool-timeout-issue.patch \
	    file://0055-Fixes-TrustedModuleRequiredToBoot-Property-patch-iss.patch \
	    file://0056-Added-Bios-Setting-URI-to-Bios.patch \
            file://0062-Fixed-VirtualMedia-not-listing-issue-under-Accounts.patch \ 
            file://0064-Fix-for-Empty-response-body-for-updating-username.patch \
            file://0066-DateTime-patch-error.patch \
            file://0068-Dmtf-Tools.Redfish-Service-Validator-getting.patch \
            file://0069-changing-the-error-code-of-non-writeable-error-messa.patch \
	    file://0070-Adding-successResponse-for-Factory-Default-Reset.patch \
	    file://0071-Added-new-property-PasswordChangeRequired-to-create-newuser.patch \
	    file://0074-Adding-400-Bad-request-response-for-invalid-MACAddre.patch \
            file://0075-removing-getcertificate-call-from-replace-certificat.patch \
"
SRC_URI_BHS:append ="file://0028-Adding-proper-path-to-get-the-cupsensors.patch \
"
SRC_URI:append = "${@bb.utils.contains('BBFILE_COLLECTIONS', 'bhs', "${@bb.utils.contains('BBFILE_COLLECTIONS', 'restricted', SRC_URI_BHS, '', d)}", '', d)}"
EXTRA_OEMESON += "${@bb.utils.contains('IMAGE_FSTYPES', 'intel-pfr', '',' -Dhttp-body-limit=68 ', d)}"

DEPENDS += "phosphor-snmp"
