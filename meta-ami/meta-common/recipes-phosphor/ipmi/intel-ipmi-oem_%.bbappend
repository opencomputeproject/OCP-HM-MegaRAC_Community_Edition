FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "b37abfb27f14c05f8cb366c5e382fe66595dd5bf"

SRC_URI += "\
	   file://0007-Change-Privilege-to-system-interface.patch \
	   file://0008-fix-sdr-count-issue.patch \
           file://0009-Removed-SetSelTime-ipmi-Handler.patch \
	   file://0010-enable-warm-reset-dcmi-and-smtp-commands.patch \
           file://0011-For-GetSensorType-ipmi-command-issue-is-getting-resp.patch \
	   file://0012-Add-SDR-Support-for-Processor-Type-Sensor.patch \
	   file://0014-Add-Watchdog2-Discrete-Sensor.patch \
	   file://0015-Add-OOB-BIOS-support-OEM.patch \	
	   file://0016-Add-SMTP-IPMI-OEM-Commands-Support.patch \
	   file://0018-fixed-add-sel.patch \
	   file://0019-fix-platform-event-ipmi-command.patch\
	   file://0020-fixed-redfish-clear-sel.patch \
       file://0001-accessing-Chassis-Force-Identity-reserved-bits.patch \
       file://0021-Fix-add-SEL-entry-IPMI-command-response.patch \
       file://0022-Add-IPMI-Get-Set-SEL-Policy-OEM-command.patch \
       file://0023-Get-SDR-with-the-Invalid-Record-ID-shows-invalid-req.patch \
     "

