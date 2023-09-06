FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"


SRC_URI += "file://plugins.d/arpcntlconf \
	    file://plugins.d/arptableinfo  \
	    file://plugins.d/biospostcode  \
	    file://plugins.d/channelaccess  \
	    file://plugins.d/channelconfig  \
	    file://plugins.d/freemem  \
	    file://plugins.d/interrupts  \
	    file://plugins.d/iproute  \
	    file://plugins.d/kernalRingBuff  \
	    file://plugins.d/kernlcmdline  \
	    file://plugins.d/mountinfo  \
	    file://plugins.d/netstat  \
	    file://plugins.d/networkconfig  \
	    file://plugins.d/networkrouteinfo  \
	    file://plugins.d/pslist  \
	    file://plugins.d/selinfo  \
	    file://plugins.d/sensorread  \
	    file://plugins.d/slabinfo  \
	    file://plugins.d/softIRQs  \
	    file://plugins.d/tmpfilelist  \
	    file://plugins.d/varfilelist  \
	   "

do_install:append() {
    install -d ${D}${dreport_plugin_dir}
    install -m 0755 ${WORKDIR}/plugins.d/* ${D}${dreport_plugin_dir}/
}

