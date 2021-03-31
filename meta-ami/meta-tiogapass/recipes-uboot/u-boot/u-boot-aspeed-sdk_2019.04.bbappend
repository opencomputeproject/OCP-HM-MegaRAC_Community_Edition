FILESEXTRAPATHS_append := "${THISDIR}/:"

SRC_URI += "file://001-common-board_r.c.patch"
SRC_URI += "file://002-tiagopass.c.patch"
SRC_URI += "file://003-board-aspeed-evb_ast2500-Makefile.patch"
SRC_URI += "file://004-ast2500-evb.dts.patch"
SRC_URI += "file://005-Unlocked-SCU-Register.patch"
SRC_URI += "file://006-Fixed-Cant-Override-U-Boot-Issue.patch"
