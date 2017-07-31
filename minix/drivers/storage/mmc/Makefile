.include <bsd.own.mk>

.for pl in omap rpi
PROGS		+=		${pl}_mmc ${pl}_emmc
MAN.${pl}_emmc	=
MAN.${pl}_mmc	=

SRCS.${pl}_emmc	+= mmcblk.c emmc_${pl}.c
SRCS.${pl}_mmc	+= mmcblk.c mmchost_dummy.c mmchost_${pl}.c
.endfor

BINDIR= /service/

DPADD+=	${LIBBLOCKDRIVER} ${LIBSYS}
LDADD+=	-lblockdriver -lsys

.include <minix.service.mk>
