#ifndef __MINIX_FB_H_
#define __MINIX_FB_H_

#include <minix/type.h>

struct fb_fix_screeninfo {
	char id[16];		/* Identification string */
	uint16_t xpanstep;
	uint16_t ypanstep;
	uint16_t ywrapstep;
	uint32_t line_length;
	phys_bytes mmio_start;
	size_t mmio_len;
	uint16_t reserved[15];
};

struct fb_bitfield {
	uint32_t offset;
	uint32_t length;
	uint32_t msb_right;
};

struct fb_var_screeninfo {
	uint32_t xres;		/* visible resolution */
	uint32_t yres;
	uint32_t xres_virtual;	/* virtual resolution */
	uint32_t yres_virtual;
	uint32_t xoffset;		/* offset from virtual to visible */
	uint32_t yoffset;
	uint32_t bits_per_pixel;
	struct fb_bitfield red;	/* bitfield in fb mem if true color */
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;	/* transparency */
	uint16_t reserved[10];
};

#endif /* __MINIX_FB_H_ */
