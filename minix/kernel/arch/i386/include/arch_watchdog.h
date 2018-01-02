#ifndef __I386_WATCHDOG_H__
#define __I386_WATCHDOG_H__

#include "kernel/kernel.h"

struct nmi_frame {
	reg_t	eax;
	reg_t	ecx;
	reg_t	edx;
	reg_t	ebx;
	reg_t	esp;
	reg_t	ebp;
	reg_t	esi;
	reg_t	edi;
	uint16_t	gs;
	uint16_t	fs;
	uint16_t	es;
	uint16_t	ds;
	reg_t	pc;	/* arch independent name for program counter */
	reg_t	cs;
	reg_t	eflags;
};

int i386_watchdog_start(void);

#define nmi_in_kernel(f)	((f)->cs == KERN_CS_SELECTOR)

#endif /* __I386_WATCHDOG_H__ */
