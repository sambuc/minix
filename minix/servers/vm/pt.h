
#ifndef _PT_H
#define _PT_H 1

#include <machine/vm.h>

#include "vm.h"
#include "pagetable.h"

/* A pagetable. */
typedef struct {
	/* Directory entries in VM addr space - root of page table.  */
	uint32_t *pt_dir;		/* page aligned (ARCH_VM_DIR_ENTRIES) */
	uint32_t pt_dir_phys;	/* physical address of pt_dir */

	/* Pointers to page tables in VM address space. */
	uint32_t *pt_pt[ARCH_VM_DIR_ENTRIES];

	/* When looking for a hole in virtual address space, start
	 * looking here. This is in linear addresses, i.e.,
	 * not as the process sees it but the position in the page
	 * page table. This is just a hint.
	 */
	uint32_t pt_virtop;
} pt_t;

#define CLICKSPERPAGE (VM_PAGE_SIZE/CLICK_SIZE)

#endif
