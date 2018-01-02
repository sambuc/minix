#ifndef FPU_H
#define FPU_H

/* x87 FPU state, MMX Technolodgy.
 * 108 bytes.*/
struct fpu_regs_s {
	uint16_t fp_control;     /* control */
	uint16_t fp_unused_1;
	uint16_t fp_status;      /* status */
	uint16_t fp_unused_2;
	uint16_t fp_tag;         /* register tags */
	uint16_t fp_unused_3;
	uint32_t fp_eip;         /* eip at failed instruction */
	uint16_t fp_cs;          /* cs at failed instruction */
	uint16_t fp_opcode;      /* opcode of failed instruction */
	uint32_t fp_dp;          /* data address */
	uint16_t fp_ds;          /* data segment */
	uint16_t fp_unused_4;
	uint16_t fp_st_regs[8][5]; /* 8 80-bit FP registers */
};

/* x87 FPU, MMX Technolodgy and SSE state.
 * 512 bytes (if you need size use FPU_XFP_SIZE). */
struct xfp_save {
	uint16_t fp_control;       /* control */
	uint16_t fp_status;        /* status */
	uint16_t fp_tag;           /* register tags */
	uint16_t fp_opcode;        /* opcode of failed instruction */
	uint32_t fp_eip;           /* eip at failed instruction */
	uint16_t fp_cs;            /* cs at failed instruction */
	uint16_t fp_unused_1;
	uint32_t fp_dp;            /* data address */
	uint16_t fp_ds;            /* data segment */
	uint16_t fp_unused_2;
	uint32_t fp_mxcsr;         /* MXCSR */
	uint32_t fp_mxcsr_mask;    /* MXCSR_MASK */
	uint16_t fp_st_regs[8][8];   /* 128 bytes for ST/MM regs */
	uint32_t fp_xreg_word[32]; /* space for 8 128-bit XMM registers */
	uint32_t fp_padding[56];
};

/* Size of xfp_save structure. */
#define FPU_XFP_SIZE		512

union fpu_state_u {
	struct fpu_regs_s fpu_regs;
	struct xfp_save xfp_regs;
};

#endif /* #ifndef FPU_H */
