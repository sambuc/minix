.include <bsd.own.mk>

PROGS=      rpi_mmc rpi_emmc
MAN.rpi_emmc=
MAN.rpi_mmc=
BINDIR= /service/

SRCS.rpi_emmc+= emmc.c mmcblk.c

.include "../Makefile.inc"

SRCS.rpi_mmc=  ${MMC_SRCS}
SRCS.rpi_mmc += mmchost_mmchs.c 

DPADD+=	${LIBBLOCKDRIVER} ${LIBSYS}
LDADD+=	-lblockdriver -lsys
CLEANFILES+=.depend mmcblk.d

#
# This is a system driver.
CPPFLAGS+= -D_SYSTEM=1 

.include <minix.service.mk>
