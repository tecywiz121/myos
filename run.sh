#!/bin/bash

# This script can be used to quickly test MultiBoot-compliant
# kernels.

# ---- begin config params ----

harddisk_image="bootable.iso"
qemu_cmdline="kvm -monitor stdio"
kernel_args=""
kernel_binary="kernel.bin"

# ----  end config params  ----


function fail() { echo "$1"; exit 1; }
function prereq() {
	local c x
	if [ "$1" = "f" ]; then c=stat;x=file; else c=which;x=program; fi
	if [ -z "$3" ]; then
		$c "$2" >/dev/null || fail "$x $2 not found"
	else
		$c "$2" >/dev/null || fail "$x $2 (from package $3) not found"
	fi
}

# check prerequisites
prereq x grub-mkrescue grub2
prereq x xorriso xorriso


# move kernel into iso source tree
cp "$kernel_binary" "iso/boot/"

# format image
grub-mkrescue -o "$harddisk_image" iso || fail "could not create bootable iso"

# run QEMU
$qemu_cmdline "$@" -boot d -cdrom "$harddisk_image" -d int,cpu_reset

echo
