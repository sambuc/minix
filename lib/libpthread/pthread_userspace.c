#include <sys/cdefs.h>

#include <sys/lwp.h>
#include <sys/param.h>
#include <sys/ras.h>

#include <lwp.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "pthread.h"
#include "pthread_int.h"

#define MAX_THREAD_POOL 1024
#define SLOT_IN_USE 0x0001

int	_sys_sched_yield(void);
int	_sys_mq_send(mqd_t, const char *, size_t, unsigned);
ssize_t	_sys_mq_receive(mqd_t, char *, size_t, unsigned *);

/* Work around kludge for pthread_cancelstub */
int pthread__cancel_stub_binder;

#if 0
#define print(msg) \
	{ \
		const char m[] = msg; \
		write(2, m, sizeof(m)); \
	}
#else
#define print(m) /**/
#endif

extern void
__minix_schedule(int signal __unused);

void __pthread_init_minix(void) __attribute__((__constructor__, __used__));
void 
__pthread_init_minix(void)
{
	int r;
        static struct sigaction old_action;
        static struct sigaction new_action;

        struct itimerval nit;
        struct itimerval oit;

	memset(&old_action, 0, sizeof(old_action));
	memset(&new_action, 0, sizeof(new_action));

        new_action.sa_handler = __minix_schedule;
        new_action.sa_flags = 0;
        r = sigaction(SIGVTALRM, &new_action, &old_action);

        nit.it_value.tv_sec = 0;
        nit.it_value.tv_usec = 1;
        nit.it_interval.tv_sec = 0;
        nit.it_interval.tv_usec = 10;
        r = setitimer(ITIMER_VIRTUAL, &nit, &oit);
}

int
_sys_sched_yield(void)
{
	__minix_schedule(SIGVTALRM);
	return -1;
}

int
sched_yield(void)
{
	return _sys_sched_yield();
}

#if 1 /* FIXME */
int
_sched_setaffinity(pid_t a, lwpid_t b, size_t c, const cpuset_t *d)
{
	print("_sched_setaffinity\n");
	return -1;
}

int
_sched_getaffinity(pid_t a, lwpid_t b, size_t c, cpuset_t *d)
{
	print("_sched_getaffinity\n");
	return -1;
}

int
_sched_setparam(pid_t a, lwpid_t b, int c, const struct sched_param *d)
{
	print("_sched_setparam\n");
	return -1;
}

int
_sched_getparam(pid_t a, lwpid_t b, int *c, struct sched_param *d)
{
	print("_sched_getparam\n");
	return -1;
}

int
rasctl(void *addr, size_t len, int op)
{
	print("rasctl\n");
	errno = EOPNOTSUPP;
	return -1;
}

#if 0
int
_sys_mq_send(mqd_t a, const char *b, size_t c, unsigned d)
{
	return -1;
}

ssize_t
_sys_mq_receive(mqd_t a, char *b, size_t c, unsigned *d)
{
	return -1;
}
#endif /* 0 */
#endif /* 0 */
