#include <sys/cdefs.h>
#include "namespace.h"
#include <lib.h>

#include <string.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(fork, _fork)
/* Allow libc to override with definition from pthread_atfork, while defining
 * the symbol for libminc */
__weak_alias(_fork, __fork)
#endif

pid_t __fork(void); /* LSC, quiet prototype not found warning. */

pid_t __fork(void)
{
  message m;

  memset(&m, 0, sizeof(m));
  return(_syscall(PM_PROC_NR, PM_FORK, &m));
}
