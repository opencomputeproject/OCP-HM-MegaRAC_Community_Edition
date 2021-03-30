FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

do_compile_prepend (){
        cd ${S}
        cp ../restore-default-controller.* app/server-control/controllers/
}

SRC_URI_append += "file://0001-Virtual-Media-WebUI-OSP2.1-Merge.patch \
		   file://0002-Added-Ctrl-Alt-Del-Key-Feature-KVM.patch \
                   file://0003-Added-Master-Kvm-Session-Support-Webui.patch \
		   file://0004-user-delete-update-failure-status-message.patch \
		   file://0005-made-changes-in-network-controller.patch \
		   file://0006-fix-unauthorized-pages-in-webui-eip-537635.patch \
		   file://0007-Vmedia-fix.patch \
		   file://0008-network-settings-and-overview-page-fixes.patch \
		   file://0009-date_time_settings_and_user_management_fixes.patch \
		   file://0010-fw-version-shown-and-remove-delete-after-activate.patch \
		   file://restore-default-controller.html \
		   file://restore-default-controller.js \
		   file://0011-restore-factory-default-feature.patch \
		   file://0012-removed-time-owner-in-date-and-time.patch \
                   file://0013-maxtries-error-msg.patch \
                   file://0014-VM-restrict-irrespective-file-format.patch \
		   file://0015-webui_fixes_for_TB2.patch \
		   file://0016-getNetworkInfoPromise-for-VM-used.patch \
		   file://0017-Fixed-VM-reload-browser-issue.patch \
		   file://0018-NTP-and-Privilege_issue.patch \
		   file://0019-Fixed-KVM-Open-Tab-Issue.patch \
		   file://0020-fixed-network-settings-user-privilege-and-logs-issue.patch \
		   file://0021-Fixed-DefaultGateway-Issue.patch \
		   file://0022-NTP-manual-time-setting-issue.patch \
		   file://0023-Logs-date-filter-user-lock-timeout-issue.patch \
		   file://0024-Hardware-status-list-and-server-health-issue.patch \
		   file://0025-Date-and-Time-year-issue.patch \
		   file://0026-Fixed-KVM-Logout-Popup-Warning-issue.patch \
                  "
