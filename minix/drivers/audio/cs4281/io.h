#ifndef _IO_H
#define _IO_H

#include <sys/types.h>
#include <minix/syslib.h>
#include "cs4281.h"

/* I/O function */
static uint8_t my_inb(uint32_t port) {
	uint32_t value;
	int r;
#ifdef DMA_BASE_IOMAP
	value = *(volatile uint8_t *)(port);
#else
	if ((r = sys_inb(port, &value)) != OK)
		printf("SDR: sys_inb failed: %d\n", r);
#endif
	return (uint8_t)value;
}
#define sdr_in8(port, offset) (my_inb((port) + (offset)))

static uint16_t my_inw(uint32_t port) {
	uint32_t value;
	int r;
#ifdef DMA_BASE_IOMAP
	value = *(volatile uint16_t *)(port);
#else
	if ((r = sys_inw(port, &value)) != OK)
		printf("SDR: sys_inw failed: %d\n", r);
#endif
	return (uint16_t)value;
}
#define sdr_in16(port, offset) (my_inw((port) + (offset)))

static uint32_t my_inl(uint32_t port) {
	uint32_t value;
	int r;
#ifdef DMA_BASE_IOMAP
	value = *(volatile uint32_t *)(port);
#else
	if ((r = sys_inl(port, &value)) != OK)
		printf("SDR: sys_inl failed: %d\n", r);
#endif
	return value;
}
#define sdr_in32(port, offset) (my_inl((port) + (offset)))

static void my_outb(uint32_t port, uint32_t value) {
	int r;
#ifdef DMA_BASE_IOMAP
	*(volatile uint8_t *)(port) = value;
#else
	if ((r = sys_outb(port, (uint8_t)value)) != OK)
		printf("SDR: sys_outb failed: %d\n", r);
#endif
}
#define sdr_out8(port, offset, value) \
				(my_outb(((port) + (offset)), (value)))

static void my_outw(uint32_t port, uint32_t value) {
	int r;
#ifdef DMA_BASE_IOMAP
	*(volatile uint16_t *)(port) = value;
#else
	if ((r = sys_outw(port, (uint16_t)value)) != OK)
		printf("SDR: sys_outw failed: %d\n", r);
#endif
}
#define sdr_out16(port, offset, value) \
				(my_outw(((port) + (offset)), (value)))

static void my_outl(uint32_t port, uint32_t value) {
	int r;
#ifdef DMA_BASE_IOMAP
	*(volatile uint32_t *)(port) = value;
#else
	if ((r = sys_outl(port, value)) != OK)
		printf("SDR: sys_outl failed: %d\n", r);
#endif
}
#define sdr_out32(port, offset, value) \
				(my_outl(((port) + (offset)), (value)))

#endif
