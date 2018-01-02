/*
minix/portio.h

Created:	Jan 15, 1992 by Philip Homburg
*/

#ifndef _PORTIO_H_
#define _PORTIO_H_

#include <sys/types.h>

unsigned inb(uint16_t _port);
unsigned inw(uint16_t _port);
unsigned inl(uint16_t _port);
void outb(uint16_t _port, uint8_t _value);
void outw(uint16_t _port, uint16_t _value);
void outl(uint16_t _port, u32_t _value);
void intr_disable(void);
void intr_enable(void);

#endif /* _PORTIO_H_ */
