FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI_append += "file://0001.change_pwm_dutycycle_value_range.patch"
SRC_URI_append += "file://0002-pwm-max-min-value-range.patch"
SRC_URI_append += "file://0003-change_pwm_dutycycle_value.patch"
