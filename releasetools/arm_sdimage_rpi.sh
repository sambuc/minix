#!/usr/bin/env bash
set -e

#
# This script creates a bootable image and should at some point in the future
# be replaced by the proper NetBSD infrastructure.
#

#
# Source settings if present
#
: ${SETTINGS_MINIX=.settings}
if [ -f "${SETTINGS_MINIX}"  ]
then
	echo "Sourcing settings from ${SETTINGS_MINIX}"
	# Display the content (so we can check in the build logs
	# what the settings contain.
	cat ${SETTINGS_MINIX} | sed "s,^,CONTENT ,g"
	. ${SETTINGS_MINIX}
fi

BSP_NAME=rpi
: ${ARCH=evbearm-el}
: ${TOOLCHAIN_TRIPLET=arm-elf32-minix-}
: ${BUILDSH=build.sh}

: ${SETS="minix-base minix-comp minix-games minix-man minix-tests tests"}
: ${IMG=minix_arm_sd.img}

# ARM definitions:
: ${BUILDVARS=-V MKGCCCMDS=yes -V MKLLVM=no}
# These BUILDVARS are for building with LLVM:
#: ${BUILDVARS=-V MKLIBCXX=no -V MKKYUA=no -V MKATF=no -V MKLLVMCMDS=no}
: ${FAT_SIZE=$((    64*(2**20) / 512))} # This is in sectors

if [ ! -f ${BUILDSH} ]
then
	echo "Please invoke me from the root source dir, where ${BUILDSH} is."
	exit 1
fi

export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:${PATH}

# we create a disk image of about 2 gig's
# for alignment reasons, prefer sizes which are multiples of 4096 bytes
: ${IMG_SIZE=$((     2*(2**30) ))}
: ${ROOT_SIZE=$((   64*(2**20) ))}
: ${HOME_SIZE=$((  128*(2**20) ))}
: ${USR_SIZE=$((  1792*(2**20) ))}

# set up disk creation environment
. releasetools/image.defaults
. releasetools/image.functions

# all sizes are written in 512 byte blocks
ROOTSIZEARG="-b $((${ROOT_SIZE} / 512 / 8))"
USRSIZEARG="-b $((${USR_SIZE} / 512 / 8))"
HOMESIZEARG="-b $((${HOME_SIZE} / 512 / 8))"

# where the kernel & boot modules will be
MODDIR=${DESTDIR}/boot/minix/.temp

echo "Building work directory..."
build_workdir "$SETS"

echo "Adding extra files..."

# create a fstab entry in /etc
cat >${ROOT_DIR}/etc/fstab <<END_FSTAB
/dev/c0d0p2	/usr		mfs	rw			0	2
/dev/c0d0p3	/home		mfs	rw			0	2
none		/sys		devman	rw,rslabel=devman	0	0
none		/dev/pts	ptyfs	rw,rslabel=ptyfs	0	0
END_FSTAB
add_file_spec "etc/fstab" extra.fstab

echo "Bundling packages..."
bundle_packages "$BUNDLE_PACKAGES"

echo "Creating specification files..."
create_input_spec
create_protos "usr home"

# Download the stage 1 bootloader and u-boot
#
${RELEASETOOLSDIR}/checkout_repo.sh -o ${RELEASETOOLSDIR}/rpi-firmware -b ${RPI_FIRMWARE_BRANCH} -n ${RPI_FIRMWARE_REVISION} ${RPI_FIRMWARE_URL}

# Clean image
if [ -f ${IMG} ]	# IMG might be a block device
then
	rm -f ${IMG}
fi

#
# Create the empty image where we later will put the partitions in.
# Make sure it is at least 2GB, otherwise the SD card will not be detected
# correctly in qemu / HW.
#
dd if=/dev/zero of=${IMG} bs=512 count=1 seek=$((($IMG_SIZE / 512) -1))

#
# Generate /root, /usr and /home partition images.
#
echo "Writing disk image..."
FAT_START=$((4096 / 512)) # those are sectors
ROOT_START=$(($FAT_START + $FAT_SIZE))
echo " * ROOT"
_ROOT_SIZE=$(${CROSS_TOOLS}/nbmkfs.mfs -d ${ROOTSIZEARG} -I $((${ROOT_START}*512)) ${IMG} ${WORK_DIR}/proto.root)
_ROOT_SIZE=$(($_ROOT_SIZE / 512))
USR_START=$((${ROOT_START} + ${_ROOT_SIZE}))
echo " * USR"
_USR_SIZE=$(${CROSS_TOOLS}/nbmkfs.mfs  -d ${USRSIZEARG}  -I $((${USR_START}*512))  ${IMG} ${WORK_DIR}/proto.usr)
_USR_SIZE=$(($_USR_SIZE / 512))
HOME_START=$((${USR_START} + ${_USR_SIZE}))
echo " * HOME"
_HOME_SIZE=$(${CROSS_TOOLS}/nbmkfs.mfs -d ${HOMESIZEARG} -I $((${HOME_START}*512)) ${IMG} ${WORK_DIR}/proto.home)
_HOME_SIZE=$(($_HOME_SIZE / 512))
echo " * BOOT"
rm -rf ${ROOT_DIR}/*
# copy device trees
cp -r releasetools/rpi-firmware/* ${ROOT_DIR}

# Write GPU config file
cat <<EOF >${ROOT_DIR}/config.txt
[pi3]
device_tree=bcm2710-rpi-3-b.dtb
enable_uart=1
dtoverlay=pi3-disable-bt

[pi2]
device_tree=bcm2709-rpi-2-b.dtb

[all]
device_tree_address=0x100
kernel=minix_rpi.bin

dtparam=i2c_arm=on
EOF

#
# Do some last processing of the kernel and servers and then put them on the FAT
# partition.
#

# copy over all modules
for i in ${MODDIR}/*
do
	cp $i ${WORK_DIR}/$(basename $i).elf
done
${CROSS_PREFIX}objcopy ${OBJ}/minix/kernel/kernel -O binary ${WORK_DIR}/kernel.bin

(
	# create packer
	export CROSS_PREFIX=$PWD/${CROSS_PREFIX}
	${CROSS_PREFIX}as ${RELEASETOOLSDIR}/rpi-bootloader/bootloader.S -o ${WORK_DIR}/bootloader.o

	cd ${WORK_DIR}
	${CROSS_PREFIX}ld bootloader.o -o bootloader.elf -Ttext=0x8000
	${CROSS_PREFIX}objcopy -O binary bootloader.elf ${ROOT_DIR}/minix_rpi.bin

	# pack modules
	cpio -o --format=newc >> ${ROOT_DIR}/minix_rpi.bin <<EOF
kernel.bin
mod01_ds.elf
mod02_rs.elf
mod03_pm.elf
mod04_sched.elf
mod05_vfs.elf
mod06_memory.elf
mod07_tty.elf
mod08_mib.elf
mod09_vm.elf
mod10_pfs.elf
mod11_mfs.elf
mod12_init.elf
EOF
)

cat >${WORK_DIR}/boot.mtree <<EOF
. type=dir
./LICENCE.broadcom type=file
./bcm2708-rpi-b-plus.dtb type=file
./bcm2708-rpi-b.dtb type=file
./bcm2708-rpi-cm.dtb type=file
./bcm2709-rpi-2-b.dtb type=file
./bcm2710-rpi-3-b.dtb type=file
./bootcode.bin type=file
./config.txt type=file
./fixup.dat type=file
./minix_rpi.bin type=file
./overlays type=dir
./overlays/pi3-disable-bt.dtbo type=file
./overlays/pi3-miniuart-bt.dtbo type=file
./start.elf type=file
EOF

#
# Create the FAT partition, which contains the bootloader files, kernel and modules
#
${CROSS_TOOLS}/nbmakefs -t msdos -s ${FAT_SIZE}b -o F=32,c=1 \
	-F ${WORK_DIR}/boot.mtree ${WORK_DIR}/fat.img ${ROOT_DIR}

#
# Write the partition table using the natively compiled
# minix partition utility
#
${CROSS_TOOLS}/nbpartition -f -m ${IMG} ${FAT_START} \
	"c:${FAT_SIZE}*" 81:${_ROOT_SIZE} 81:${_USR_SIZE} 81:${_HOME_SIZE}

#
# Merge the partitions into a single image.
#
echo "Merging file systems"
dd if=${WORK_DIR}/fat.img of=${IMG} seek=$FAT_START conv=notrunc

echo "Disk image at `pwd`/${IMG}"
echo "To boot this image on kvm:"
echo "qemu-system-arm -M raspi2 -kernel if=sd,cache=writeback,format=raw,file=/$(pwd)/${IMG} -bios ${ROOT_DIR}/minix_rpi.bin -serial stdio -dtb $(pwd)/releasetools/rpi-firmware/bcm2709-rpi-2-b.dtb "
