#include <sys/cdefs.h>

#include <sys/aio.h>
#include <sys/lwp.h>
#include <sys/lwpctl.h>
#include <sys/param.h>
#include <sys/ras.h>
#include <sys/syscall.h>

#include <assert.h>
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

void	__libc_thr_init(void);
void	__libc_atomic_init(void);

int	_sys_sched_yield(void);
int	_sys_mq_send(mqd_t, const char *, size_t, unsigned);
ssize_t	_sys_mq_receive(mqd_t, char *, size_t, unsigned *);

/* Work around kludge for pthread_cancelstub */
int pthread__cancel_stub_binder;

struct lwp {
	int return_value;
	int flags;
	const char * name;
	ucontext_t context;
	struct tls_tcb tls;
};

static struct lwp lwp_threads[MAX_THREAD_POOL];
static volatile lwpid_t current_thread = 0;

static int
__minix_runnable(struct lwp* thread)
{
	uint32_t mask = (LWP_SUSPENDED | LSSLEEP | LW_UNPARKED | SLOT_IN_USE);
	uint32_t runnable = (~LWP_SUSPENDED | ~LSSLEEP | LW_UNPARKED | SLOT_IN_USE);

	if ((thread->flags & mask) == runnable) {
		return 1; /* Runnable */
	}

	return 0; /* Not runnable */
}

static void
__minix_schedule(int signal __unused)
{
	static int last_pos = 0;
	struct lwp* old = &lwp_threads[current_thread];
	int pos;

	/* Select Next thread to run. 
	 * Simply scan the array looking for a schedulable thread, and
	 * loopback to the start if we reach the end. */
	for(pos = last_pos; pos != last_pos;
	    pos = ((pos + 1) % MAX_THREAD_POOL)) {
		if (__minix_runnable(&lwp_threads[pos])) {
			break;
		}
	}

	if (pos == last_pos) {
		/* No other thread found to run, is the current one
		 * still runnable? */	
		if (!__minix_runnable(&lwp_threads[pos])) {
			return; /* "No runnable threads to schedule. */
		}
	}

	/* Point the current thread to the thread picked. */
	current_thread = pos;

	/* Restore the next context of the thread picked. */
	last_pos = pos;

	(void)swapcontext(&(old->context), &(lwp_threads[pos].context));
}

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

lwpid_t
_lwp_self(void)
{
	return current_thread;
}

void
_lwp_makecontext(ucontext_t *context, void (*start_routine)(void *),
         void *arg, void *private, caddr_t stack_base, size_t stack_size)
{
	/* Already done in pthread_makelwp, here for reference */
	memset(context, 0, sizeof(*context));
	_INITCONTEXT_U(context);
	context->uc_stack.ss_sp = stack_base;
	context->uc_stack.ss_size = stack_size;
	context->uc_stack.ss_flags = 0;
	context->uc_link = NULL;

	makecontext(context, start_routine, 1, arg);
}

int
_lwp_create(const ucontext_t *context, unsigned long flags, lwpid_t *new_lwp)
{
	size_t i = 0;

	while ((i < MAX_THREAD_POOL) &&
		(SLOT_IN_USE == (lwp_threads[i].flags & SLOT_IN_USE))) {
		i++;
	}

	if (MAX_THREAD_POOL == i) {
		errno = EAGAIN;
		return -1;
	}

	/* ADD CHECKS ON UCONTEXT */

	memset(&lwp_threads[i], 0, sizeof(lwp_threads[i]));
	lwp_threads[i].flags = flags | SLOT_IN_USE;
	lwp_threads[i].context = *context;
	*new_lwp = i;

	return 0;
}

int
_lwp_suspend(lwpid_t lwp)
{
	int nb_lwp = 0;

	for (size_t i = 0; i < MAX_THREAD_POOL; i++) {
		if (SLOT_IN_USE == (lwp_threads[i].flags & SLOT_IN_USE)) {
			nb_lwp++;
		}
	}

	if (1 < nb_lwp) {
		return EDEADLK;
	}

	if ((MAX_THREAD_POOL <= lwp) || (lwp < 0)) {
		return ESRCH;
	}
	
	lwp_threads[lwp].flags |= LW_WSUSPEND;

	return 0;
}

int
_lwp_continue(lwpid_t lwp)
{
	if ((MAX_THREAD_POOL <= lwp) || (lwp < 0)) {
		return ESRCH;
	}
	
	lwp_threads[lwp].flags &= ~LW_WSUSPEND;

	return 0;
}

int
_lwp_wakeup(lwpid_t lwp)
{
	if ((MAX_THREAD_POOL <= lwp) || (lwp < 0)) {
		return ESRCH;
	}

	if (LSSLEEP != (lwp_threads[lwp].flags & LSSLEEP)) {
		return ENODEV;
	}
	
	lwp_threads[lwp].flags &= ~LSSLEEP;

	__minix_schedule(SIGVTALRM);

	return 0;
}

int
_lwp_detach(lwpid_t lwp)
{
	if ((MAX_THREAD_POOL <= lwp) || (lwp < 0)) {
		errno = ESRCH;
		return -1;
	}

	if (LWP_DETACHED == (lwp_threads[lwp].flags & LWP_DETACHED)) {
		errno = EINVAL;
		return -1;
	}
	
	lwp_threads[lwp].flags |= LWP_DETACHED;

	return 0;
}

int
_lwp_setname(lwpid_t target, const char * name)
{
	/* Name is already a copy for our use. */
	lwp_threads[target].name = name;

	return 0;
}

int
_lwp_getname(lwpid_t target, char * name, size_t len)
{
	return strlcpy(name, lwp_threads[target].name, len) < len ? 0 : -1;
}

void *
_lwp_getprivate(void)
{
	return &lwp_threads[current_thread].tls;
}

void
_lwp_setprivate(void *cookie)
{
	/* Not supported */
}

int
_lwp_unpark(lwpid_t lwp, const void * hint)
{
	if ((MAX_THREAD_POOL <= lwp) || (lwp < 0)) {
		errno = ESRCH;
		return -1;
	}

	lwp_threads[lwp].flags |= LW_UNPARKED;

	return 0;
}

ssize_t
_lwp_unpark_all(const lwpid_t * targets, size_t ntargets, const void * hint)
{
	if (NULL == targets) {
		return MAX_THREAD_POOL;
	}

	if (MAX_THREAD_POOL <= ntargets) {
		errno = EINVAL;
		return -1;
	}

	for (size_t i = 0; i < ntargets; i++) {
		lwp_threads[targets[i]].flags |= LW_UNPARKED;
	}

	return 0;
}

int
_lwp_park(clockid_t clock_id, int i, const struct timespec * ts, lwpid_t thread,
    const void *cookie, const void *cookie2)
{
	// FIXME
	return -1;
}

int
_lwp_wait(lwpid_t wlwp, lwpid_t *rlwp)
{
	// FIXME
	return -1;
}

int
_lwp_kill(lwpid_t thread, int signal)
{
	// FIXME
	errno = ESRCH;
	return -1;
}

int
_lwp_exit(void)
{
	lwp_threads[current_thread].flags &= ~SLOT_IN_USE;
	__minix_schedule(SIGVTALRM);

	/* We reach this only if there is nothing left to schedule. */
	exit(0);
}

int
_lwp_ctl(int features, struct lwpctl **address)
{
	/* LSC Add stuff to actually do something with this. */
	*address = malloc(sizeof(struct lwpctl));
	if (NULL == *address) {
		return -1;
	}

	memset(*address, 0, sizeof(struct lwpctl));

	// FIXME
	return 0;
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
	return -1;
}

int
_sched_getaffinity(pid_t a, lwpid_t b, size_t c, cpuset_t *d)
{
	return -1;
}

int
_sched_setparam(pid_t a, lwpid_t b, int c, const struct sched_param *d)
{
	return -1;
}

int
_sched_getparam(pid_t a, lwpid_t b, int *c, struct sched_param *d)
{
	return -1;
}

int
rasctl(void *addr, size_t len, int op)
{
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
