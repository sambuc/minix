
#ifndef _ARM_TYPES_H
#define _ARM_TYPES_H

#include <minix/sys_config.h>
#include <machine/stackframe.h>
#include <sys/cdefs.h>

typedef struct segframe {
	reg_t	p_ttbr;		/* page table root */
	uint32_t	*p_ttbr_v;
	char	*fpu_state;
} segframe_t;

struct cpu_info {
	uint32_t	arch;
	uint32_t	implementer;
	uint32_t	part;
	uint32_t	variant;
	uint32_t	freq;		/* in MHz */
	uint32_t	revision;
};

typedef uint32_t atomic_t;	/* access to an aligned 32bit value is atomic on ARM */

#endif /* #ifndef _ARM_TYPES_H */

