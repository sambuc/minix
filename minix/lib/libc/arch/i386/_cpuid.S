/* _cpuid() - interface to cpuid instruction */


/* void _cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx); */
/* 0 for OK, nonzero for unsupported */

#include <machine/asm.h>

ENTRY(_cpuid)
/* save work registers */
	push	%ebp
	push	%ebx

/* set eax parameter to cpuid and execute cpuid */
	movl	12(%esp), %ebp
	mov	(%ebp), %eax
	movl	16(%esp), %ebp
	mov	(%ebp), %ebx
	movl	20(%esp), %ebp
	mov	(%ebp), %ecx
	movl	24(%esp), %ebp
	mov	(%ebp), %edx

.byte	0x0F, 0xA2	/* CPUID */

/* store results in pointer arguments */
	movl	12(%esp), %ebp
	movl	%eax, (%ebp)
	movl	16(%esp), %ebp
	movl	%ebx, (%ebp)
	movl	20(%esp), %ebp
	movl	%ecx, (%ebp)
	movl	24(%esp), %ebp
	movl	%edx, (%ebp)

/* restore registers */
	pop	%ebx
	pop	%ebp

	ret
