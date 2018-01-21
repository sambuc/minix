#include <sys/cdefs.h>

#include <sys/lwp.h>
#include <sys/lwpctl.h>
#include <sys/param.h>

#include <lwp.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MAX_THREAD_POOL 1024
#define SLOT_IN_USE 0x0001

struct lwp {
	int return_value;
	int flags;
	const char * name;
	ucontext_t context;
	struct tls_tcb *tls;
};

static struct lwp lwp_threads[MAX_THREAD_POOL];
static volatile lwpid_t current_thread = 0;
static int initialized = 0;

void __minix_setup_main(void);
void __minix_schedule(int signal __unused);

#if 0
#define print(args...) \
{ \
	char buffer[200]; \
	snprintf(buffer, 200, args); \
	write(2, buffer, strlen(buffer)); \
}
#else
#define print(m...) /**/
#endif

static int
__minix_runnable(struct lwp* thread)
{
	uint32_t mask = (LWP_SUSPENDED | LSSLEEP | LW_UNPARKED | SLOT_IN_USE);
	uint32_t runnable = (LW_UNPARKED | SLOT_IN_USE);

	if ((thread->flags & mask) == runnable) {
		return 1; /* Runnable */
	}

	return 0; /* Not runnable */
}

void
__minix_setup_main(void)
{
	lwp_threads[0].flags = SLOT_IN_USE | LW_UNPARKED;
}

void
__minix_schedule(int signal __unused)
{
	extern int __isthreaded;
	static int last_pos = 0;
	struct lwp* old = &lwp_threads[current_thread];
	int pos;

	/* This will be set to 1 in pthread.c, when the main thread is ready. */
	if (0 == __isthreaded)
	{
		return;
	}

//FIXME: LSC ADD A BARRIER TO PREVENT TWO EXECUTIONS OF THE SCHEDULE AT THE SAME TIME

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
			print("__minix_schedule no switch\n");
			return; /* "No runnable threads to schedule. */
		}
	}

	print("__minix_schedule switch\n");
	/* Point the current thread to the thread picked. */
	current_thread = pos;

	/* Restore the next context of the thread picked. */
	last_pos = pos;

	(void)swapcontext(&(old->context), &(lwp_threads[pos].context));
}

lwpid_t
_lwp_self(void)
{
	return current_thread;
}
#if 0
void
_lwp_makecontext(ucontext_t *context, void (*start_routine)(void *),
         void *arg, void *private, caddr_t stack_base, size_t stack_size)
{
	/* Already done in pthread_makelwp, here for reference */
	memset(context, 0, sizeof(*context));
	context->uc_flags = _UC_CPU | _UC_STACK;;
	context->uc_stack.ss_sp = stack_base;
	context->uc_stack.ss_size = stack_size;
	context->uc_stack.ss_flags = 0;
	context->uc_link = NULL;

	makecontext(context, start_routine, 1, arg);
}
#endif
int
_lwp_create(const ucontext_t *context, unsigned long flags, lwpid_t *new_lwp)
{
	size_t i = 1; // Skip slot 0 which is main thread

	print("_lwp_create\n");
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

	print("_lwp_suspend\n");
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
	print("_lwp_continue\n");
	if ((MAX_THREAD_POOL <= lwp) || (lwp < 0)) {
		return ESRCH;
	}
	
	lwp_threads[lwp].flags &= ~LW_WSUSPEND;

	return 0;
}

int
_lwp_wakeup(lwpid_t lwp)
{
	print("_lwp_wakeup\n");
	if ((MAX_THREAD_POOL <= lwp) || (lwp < 0)) {
		return ESRCH;
	}

	print("_lwp_wakeup 2\n");
	if (LSSLEEP != (lwp_threads[lwp].flags & LSSLEEP)) {
		return ENODEV;
	}
	
	lwp_threads[lwp].flags &= ~LSSLEEP;

	print("_lwp_wakeup 3\n");
	__minix_schedule(SIGVTALRM);

	print("_lwp_wakeup 4\n");
	return 0;
}

int
_lwp_detach(lwpid_t lwp)
{
	print("_lwp_detach\n");
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
	print("_lwp_setname\n");
	/* Name is already a copy for our use. */
	lwp_threads[target].name = name;

	return 0;
}

int
_lwp_getname(lwpid_t target, char * name, size_t len)
{
	print("_lwp_getname\n");
	return strlcpy(name, lwp_threads[target].name, len) < len ? 0 : -1;
}

void *
_lwp_getprivate(void)
{
//	print("_lwp_getprivate %08x %08x %08x\n", lwp_threads, &lwp_threads[current_thread], lwp_threads[current_thread].tls);
	return lwp_threads[current_thread].tls;
}

void
_lwp_setprivate(void *cookie)
{
//	print("_lwp_setprivate %08x\n", cookie);
	lwp_threads[current_thread].tls = cookie;
//	print("_lwp_setprivate %08x %08x %08x %08x\n", lwp_threads, &lwp_threads[current_thread], lwp_threads[current_thread].tls, cookie);
}

int
_lwp_unpark(lwpid_t lwp, const void * hint)
{
	print("_lwp_unpark\n");
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
	print("_lwp_unpark_all\n");
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
	print("_lwp_park\n");
	// FIXME
	return -1;
}

int
_lwp_wait(lwpid_t wlwp, lwpid_t *rlwp)
{
	print("_lwp_wait\n");
	// FIXME
	return -1;
}

int
_lwp_kill(lwpid_t thread, int signal)
{
	print("_lwp_kill\n");
	// FIXME
	errno = ESRCH;
	return -1;
}

int
_lwp_exit(void)
{
	print("_lwp_exit\n");
	lwp_threads[current_thread].flags &= ~SLOT_IN_USE;
	__minix_schedule(SIGVTALRM);

	/* We reach this only if there is nothing left to schedule. */
	exit(0);
}

int
_lwp_ctl(int features, struct lwpctl **address)
{
	print("_lwp_ctl\n");
	/* LSC Add stuff to actually do something with this. */
	*address = malloc(sizeof(struct lwpctl));
	if (NULL == *address) {
		return -1;
	}

	memset(*address, 0, sizeof(struct lwpctl));

	// FIXME
	return 0;
}
