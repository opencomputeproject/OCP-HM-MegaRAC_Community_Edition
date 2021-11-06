FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

do_compile_prepend (){
	cd ${S}
	cp ../restore-default-controller.* app/server-control/controllers/ || :
	cp ../left-arrow.svg app/assets/icons/ || :
        cp ../down-arrow.svg app/assets/icons/  || :
        cp ../right-arrow.svg app/assets/icons/ || :
        cp ../up-arrow.svg app/assets/icons/ || :
        cp ../window-icon.svg app/assets/icons/ || :
	cp ../alert-policy.scss app/configuration/styles/ || :
	cp ../event-filter-controller.* app/configuration/controllers/ || :
	cp ../smtp-controller.* app/configuration/controllers/ || :
	cp ../alert-policy-controller.* app/configuration/controllers/ || :
}

SRC_URI_append += "file://0001-made-changes-in-network-controller.patch \
		   file://0002-fix-unauthorized-pages-in-webui-eip-537635.patch \
                   file://0003-fw-version-shown-after-activate.patch \
                   file://0004-Added-Ctrl-Alt-Del-Key-Feature-KVM.patch \
                   file://0005-Added-Master-Kvm-Session-Support-Webui.patch \
                   file://0006-Virtual-Media-WebUI-OSP2.2-Merge.patch \
                   file://0007-Virtual-Media-Fix.patch \
                   file://0008-Fixed-VM-reload-browser-issue.patch \
                   file://0009-Fixed-KVM-Open-Tab-Issue.patch \
                   file://0010-Fixed-KVM-Logout-Popup-Warning-issue.patch \                   
                   file://0011-Fixed-DefaultGateway-Issue.patch \
                   file://0012-Dual-Media-Browse-For-VM-Rediretion-Support.patch \
		   file://0013-virtual-media-multiple-image-redirection-webui-changes.patch \
		   file://restore-default-controller.html \
		   file://restore-default-controller.js \
		   file://0014-WEBUI-fixes-from-2_1-to-2_2.patch \
                   file://0015-frontend-for-firmware-upgrade-settings-preservation.patch \
                   file://0016-Added-Force-Enter-BIOS-Setup-KVM-feature.patch \
		   file://0017-VLAN-WEBUI-Implementation.patch \
		   file://0018-Added-KVM-Macro.patch \
		   file://0019-expose-staticnameservers-and-dhcp.dnsenable.patch \
		   file://0020-Fixed-VM-Eject-Issue.patch \
		   file://0021-network-configuration-issues.patch \ 
		   file://0022-softkeyboard-support-with-english.patch \
		   file://left-arrow.svg \
                   file://down-arrow.svg \
		   file://right-arrow.svg \
                   file://up-arrow.svg \
                   file://window-icon.svg \
		   file://0023-IPv6-support-WEBUI.patch \
		   file://0024-Fixed-KVM-Print-Macro-Key.patch \
		   file://0025-softkeyboard-support-with-GermannItalian.patch \
		   file://0026-VM_Idle_timeout_fix_for_all_webUI_page.patch \
		   file://0027-password-policy-webui-support.patch \
		   file://0028-PEF-WEBUI-Support.patch \
		   file://0029-Password-policy-redfish-URI-changes.patch \
		   file://0030-event-log-and-VLAN-issues.patch \
		   file://0031-network-and-vlan-fixes.patch \
		   file://0032-webui-fixes-for-RC.patch \
		   file://0033-power-operation-time-issue.patch \
		   file://0034-network-and-inventory-issues-fixed.patch \
		   file://0035-webui-same-session-and-hardware-page-issue.patch \
		   file://0036-VLAN-issue.patch \
		   file://0037-network-static-IP-modification.patch \
		   file://0038-network-static-ip-address-issue.patch \
		   file://0039-network-static-ip-validation.patch \
                  "
