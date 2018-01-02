#include <stdint.h>

#include "syslib.h"

int sys_padconf(uint32_t padconf, uint32_t mask, uint32_t value)
{
        message m;

	m.PADCONF_PADCONF = padconf;
	m.PADCONF_MASK = mask;
	m.PADCONF_VALUE = value;

        return(_kernel_call(SYS_PADCONF, &m));
}
