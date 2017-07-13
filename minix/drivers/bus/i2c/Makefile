# Makefile for I2C support

PROG= i2c
SRCS+= i2c.c
FILESNAME=i2c
FILES=i2c.conf

.if ${MACHINE_ARCH} == "earm"
.for pl in rpi omap
.PATH: ${.CURDIR}/arch/earm/${pl}
SRCS+= ${pl}_i2c.c ${pl}_i2c.h ${pl}_i2c_registers.h
.endfor
.else
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}
SRCS+= pci_i2c.c pci_i2c.h pci_i2c_register.h
.endif

FILESDIR= /etc/system.conf.d
CPPFLAGS+= -I${.CURDIR} -I${.CURDIR}/arch/earm/rpi -I${.CURDIR}/arch/earm/omap
DPADD+= ${LIBCHARDRIVER} ${LIBSYS} ${LIBTIMERS} ${LIBCLKCONF}
LDADD+= -lchardriver -lsys -ltimers -lclkconf

MKMAN:= no

.include <minix.service.mk>