#!/usr/bin/env python
# SPDX-License-Identifier: GPL-2.0+
"""
A script to generate FIT image source for K3 boards

usage: $0 <board> <dt_name> [<dt_name> [<dt_name] ...]
"""

import os
import sys
import string
import re
import subprocess

core_template = string.Template("""
/dts-v1/;

/ {
	description = "Firmware image with one or more FDT blobs and overlays";
	#address-cells = <0x1>;

	images {

		u-boot {
			description = "U-Boot for $board board";
			type = "firmware";
			arch = "arm";
			os = "u-boot";
			compression = "none";
			load = <$loadaddr>;
			entry = <0x0>;
			data = /incbin/("u-boot-nodtb.bin");

		};

		$dtbs
		$overlays
	};

	configurations {
		$confs
	};
};
""")

conf_template = string.Template("""
		conf-$dtb {
			description = "$dtb";
			firmware = "u-boot";
			fdt = "$dtb.dtb";
                };
	""")

fdt_template = string.Template("""
		$basename {
			description = "$basename";
			data = /incbin/("$dtbdir/$basename");
		};
""")


def get_overlays(board):
    """
    Use strings to get the list of dtbos from the built-in.o file located
    in the board directory
    """
    overlays = []
    boarddir = os.getenv('BOARDDIR')
    r = re.compile('(.*\.dtbo)$')
    cmdline = "strings board/{}/built-in.o".format(boarddir).split()
    p = subprocess.Popen(cmdline,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    for l in p.stdout.readlines():
        m = r.match(l)
        if m:
            fname = m.group(1).strip()
            if os.path.isfile("arch/arm/dts/" + fname):
                overlays.append(fname)
            else:
                sys.stderr.write("cannot find %s. removing from its!\n" % fname)

    return overlays


def generate_confs(dtbs):
    first = True
    confs = []
    for dtb in dtbs:
        basename = dtb.split('/')[-1][:-4]
        if first:
            confs.append('default = "conf-{}";'.format(basename))
            first = False
        confs.append(conf_template.substitute(dtb=basename))
    return "\n".join(confs)


def generate_fdts(dtbos, dtbdir):
    fdts = []
    for dtbo in dtbos:
        fdts.append(fdt_template.substitute(basename=dtbo, dtbdir=dtbdir))
    return "\n".join(fdts)


def get_u_boot_test_base():
    """
    Get the text base of u-boot from the .config file
    """
    r = re.compile('CONFIG_SYS_TEXT_BASE=(.*)')
    with open('.config', 'r') as f:
        for l in f.readlines():
            m = r.search(l)
            if m:
                return m.group(1)


def generate_its(board, dtbs):
    sys_text_base = get_u_boot_test_base()
    dtbdir = '/'.join(dtbs[0].split('/')[:-1])
    dtbs = [dtb.split('/')[-1] for dtb in dtbs]

    print(core_template.substitute(
        loadaddr=sys_text_base,
        board=board,
        confs=generate_confs(dtbs),
        dtbs=generate_fdts(dtbs, dtbdir),
        overlays=generate_fdts(get_overlays(board), dtbdir))
    )


def usage():
    print(
        "{} <BOARD> <dt_name> [<dt_name> [<dt_name] ...] ".format(
            sys.argv[0]))
    sys.exit(-1)

def show_deps_and_exit():
	print("u-boot-nodtb.bin")
	print("dtbs")
	sys.exit(0)

def main():
    if len(sys.argv) < 2:
        usage()

    if sys.argv[1] == "--deps":
        show_deps_and_exit()

    board = sys.argv[1]
    dtbs = sys.argv[2:]
    generate_its(board, dtbs)


if __name__ == "__main__":
    main()
