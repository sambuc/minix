/* best viewed with tabsize 4 */

#include <minix/drivers.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


#include "pci_helper.h"

#include "es1371.h"

/*===========================================================================*
 *			helper functions for I/O										 *
 *===========================================================================*/
uint32_t pci_inb(uint16_t port) {
	uint32_t value;
	int s;
	if ((s=sys_inb(port, &value)) !=OK)
		printf("%s: warning, sys_inb failed: %d\n", DRIVER_NAME, s);
	return value;
}


uint32_t pci_inw(uint16_t port) {
	uint32_t value;
	int s;
	if ((s=sys_inw(port, &value)) !=OK)
		printf("%s: warning, sys_inw failed: %d\n", DRIVER_NAME, s);
	return value;
}


uint32_t pci_inl(uint16_t port) {
	uint32_t value;
	int s;
	if ((s=sys_inl(port, &value)) !=OK)
		printf("%s: warning, sys_inl failed: %d\n", DRIVER_NAME, s);
	return value;
}


void pci_outb(uint16_t port, uint8_t value) {
	int s;
	if ((s=sys_outb(port, value)) !=OK)
		printf("%s: warning, sys_outb failed: %d\n", DRIVER_NAME, s);
}


void pci_outw(uint16_t port, uint16_t value) {
	int s;
	if ((s=sys_outw(port, value)) !=OK)
		printf("%s: warning, sys_outw failed: %d\n", DRIVER_NAME, s);
}


void pci_outl(uint16_t port, uint32_t value) {
	int s;
	if ((s=sys_outl(port, value)) !=OK)
		printf("%s: warning, sys_outl failed: %d\n", DRIVER_NAME, s);
}

