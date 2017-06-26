.include <bsd.own.mk>

PROGS=      omap_mmc omap_emmc
MAN.omap_emmc=
MAN.omap_mmc=
BINDIR= /service/

SRCS.omap_emmc+= emmc.c mmcblk.c

.include "../Makefile.inc"

SRCS.omap_mmc=  ${MMC_SRCS} 
SRCS.omap_mmc += mmchost_mmchs.c 

DPADD+=	${LIBBLOCKDRIVER} ${LIBSYS}
LDADD+=	-lblockdriver -lsys
CLEANFILES+=.depend mmcblk.d

#
# This is a system driver.
CPPFLAGS+= -D_SYSTEM=1

.include <minix.service.mk>
