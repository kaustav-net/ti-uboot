#!/bin/sh
#
# script to generate FIT image source for K3 Family boards with
# SYSFW firmware and multiple device trees (given on the command line).
# Inspired from board/sunxi/mksunxi_fit_atf.sh
#
# usage: $0 <dt_name> [<dt_name> [<dt_name] ...]

[ -z "$SYSFW" ] && SYSFW="sysfw.bin"

if [ ! -f $SYSFW ]; then
	echo "WARNING: SYSFW firmware file $SYSFW NOT found, resulting binary is non-functional" >&2
	SYSFW=/dev/null
fi

cat << __HEADER_EOF
/dts-v1/;

/ {
	description = "Configuration to load SYSFW along with SPL";
	#address-cells = <1>;

	images {
		sysfw {
			description = "SYSFW Firmware";
			data = /incbin/("$SYSFW");
			type = "firmware";
			arch = "arm";
			compression = "none";
		};
__HEADER_EOF

for dtname in $*
do
	cat << __FDT_IMAGE_EOF
		$(basename $dtname) {
			description = "$(basename $dtname .dtb)";
			data = /incbin/("$dtname");
			type = "flat_dt";
			arch = "arm";
			compression = "none";
		};
__FDT_IMAGE_EOF
done

cat << __CONF_HEADER_EOF
	};
	configurations {
		default = "$(basename $1)";

__CONF_HEADER_EOF

for dtname in $*
do
	cat << __CONF_SECTION_EOF
		$(basename $dtname) {
			description = "$(basename $dtname .dtb)";
			firmware = "sysfw";
			fdt = "$(basename $dtname)";
		};
__CONF_SECTION_EOF
done

cat << __ITS_EOF
	};
};
__ITS_EOF
