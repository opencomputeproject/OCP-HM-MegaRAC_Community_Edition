FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
CPPFLAGS +=" -DBOOST_ASIO_DISABLE_THREADS  -DBOOST_ALL_NO_LIB -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_COROUTINES_NO_DEPRECATION_WARNING"
SRC_URI_append += "file://0001-Vm-stop-when-ip-gets-down-and-Vm-should-not-stop-when-host-getting-reboot.patch"
SRC_URI_append += "file://0002-IPSrc-change-to-tatic.patch"
SRC_URI_append += "file://0003-Keep_Configuration_DHCP_TokeepsameIP.patch"
SRC_URI_append += "file://0004-dhcp-false-after-IPSRC-to-static.patch"
SRC_URI_append += "file://0005-Restarting-NTP-timeSynchdService.patch"
SRC_URI_append += "file://0006-Dhcp-Vendor-Identifier.patch"
SRC_URI_append += "file://0007-handle-network-restart.patch"