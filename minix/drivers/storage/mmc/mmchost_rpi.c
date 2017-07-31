/* kernel headers */
#include <minix/blockdriver.h>
#include <minix/com.h>
#include <minix/vm.h>
#include <minix/spin.h>
#include <minix/log.h>
#include <minix/mmio.h>
#include <minix/type.h>
#include <minix/board.h>
#include <sys/mman.h>
#include <sys/time.h>

/* usr headers */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>

/* local headers */
#include "mmchost.h"

/* header imported from netbsd */
#include "sdmmcreg.h"
#include "sdhcreg.h"

/* rpi hardware related */
#include "mmc_rpi.h"

#define USE_INTR

#ifdef USE_INTR
static int hook_id = 1;
#endif

#define SANE_TIMEOUT 500000	/* 500 ms */

struct rpi_mmchs *mmchs;	/* pointer to the current mmchs */

struct rpi_mmchs rpi_sdcard = {
	.io_base = 0,
	.io_size = 0x100,
	.hw_base = 0x3f300000,
	.irq_nr = 126,
	.regs = &regs_v0
};

/* Integer divide x by y and ensure that the result z is
 * such that x / z is smaller or equal y
 */
#define	div_roundup(x, y) (((x)+((y)-1))/(y))

#define HSMMCSD_0_IN_FREQ	96000000 /* 96MHz */
#define HSMMCSD_0_INIT_FREQ	  400000 /* 400kHz */
#define HSMMCSD_0_FREQ_25MHZ	25000000 /* 25MHz */
#define HSMMCSD_0_FREQ_50MHZ	50000000 /* 50MHz */

void
mmc_set32(vir_bytes reg, u32_t mask, u32_t value)
{
	assert(reg >= 0 && reg <= mmchs->io_size);
	set32(mmchs->io_base + reg, mask, value);
}

u32_t
mmc_read32(vir_bytes reg)
{
	assert(reg >= 0 && reg <= mmchs->io_size);
	return read32(mmchs->io_base + reg);
}

void
mmc_write32(vir_bytes reg, u32_t value)
{
	assert(reg >= 0 && reg <= mmchs->io_size);
	write32(mmchs->io_base + reg, value);
}

void
mmchs_set_bus_freq(u32_t freq)
{
	u32_t freq_in = HSMMCSD_0_IN_FREQ;
	u32_t freq_out = freq;

	/* Calculate and program the divisor */
	u32_t clkd = div_roundup(freq_in, freq_out);
	clkd = (clkd < 2) ? 2 : clkd;
	clkd = (clkd > 1023) ? 1023 : clkd;

	log_debug(&log, "Setting divider to %d\n", clkd);
	mmc_set32(mmchs->regs->control1, MMCHS_SD_SYSCTL_CLKD, (clkd << 6));
}

/*
 * Initialize the MMC controller given a certain
 * instance. this driver only handles a single
 * mmchs controller at a given time.
 */
int
mmchs_init(uint32_t instance)
{

	uint32_t value;
	value = 0;
	struct minix_mem_range mr;
	spin_t spin;
	assert(mmchs);

	mr.mr_base = mmchs->hw_base;
	mr.mr_limit = mmchs->hw_base + mmchs->io_size;

	/* grant ourself rights to map the register memory */
	if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr) != 0) {
		panic("Unable to request permission to map memory");
	}

	/* Set the base address to use */
	mmchs->io_base =
	    (uint32_t) vm_map_phys(SELF, (void *) mmchs->hw_base,
	    mmchs->io_size);

	if (mmchs->io_base == (uint32_t) MAP_FAILED)
		panic("Unable to map MMC memory");

	/* Soft reset of the controller. This section is documented in the TRM 
	 */

	mmc_write32(mmchs->regs->control0, 0);
	mmc_write32(mmchs->regs->control2, 0);
	/* Write 1 to sysconfig[0] to trigger a reset */
	mmc_set32(mmchs->regs->control1, MMCHS_SD_SYSCTL_SRA,
		MMCHS_SD_SYSCTL_SRA);

	/* Read sysstatus to know when it's done */

	spin_init(&spin, SANE_TIMEOUT);
	while (mmc_read32(mmchs->regs->control1)
		& MMCHS_SD_SYSCTL_SRA) {
		if (spin_check(&spin) == FALSE) {
			log_warn(&log, "mmc init timeout\n");
			return 1;
		}
	}

	/* 
	 * MMC host and bus configuration
	 */

	mmc_set32(mmchs->regs->control0, MMCHS_SD_HCTL_DTW,
	    MMCHS_SD_HCTL_DTW_1BIT);

	/* Enable internal clock and clock to the card */
	mmc_set32(mmchs->regs->control1, MMCHS_SD_SYSCTL_ICE,
	    MMCHS_SD_SYSCTL_ICE_EN);

	mmchs_set_bus_freq(HSMMCSD_0_INIT_FREQ);

	mmc_set32(mmchs->regs->control1, MMCHS_SD_SYSCTL_CEN,
	    MMCHS_SD_SYSCTL_CEN_EN);

	spin_init(&spin, SANE_TIMEOUT);
	while ((mmc_read32(mmchs->regs->control1) & MMCHS_SD_SYSCTL_ICS)
	    != MMCHS_SD_SYSCTL_ICS_STABLE) {
		if (spin_check(&spin) == FALSE) {
			log_warn(&log, "clock not stable\n");
			return 1;
		}
	}

	/* 
	 * See spruh73e page 3576  Card Detection, Identification, and Selection
	 */

	/* Enable command interrupt */
	mmc_set32(mmchs->regs->irpt_mask, MMCHS_SD_IE_CC_ENABLE,
	    MMCHS_SD_IE_CC_ENABLE_ENABLE);
	/* Enable transfer complete interrupt */
	mmc_set32(mmchs->regs->irpt_mask, MMCHS_SD_IE_TC_ENABLE,
	    MMCHS_SD_IE_TC_ENABLE_ENABLE);

	/* enable error interrupts */
	mmc_set32(mmchs->regs->irpt_mask, MMCHS_SD_IE_ERROR_MASK, 0xffffffffu);

	/* clear the error interrupts */
	mmc_set32(mmchs->regs->irpt, MMCHS_SD_STAT_ERROR_MASK, 0xffffffffu);

	/* Set timeout */
	mmc_set32(mmchs->regs->control1, MMCHS_SD_SYSCTL_DTO,
	    MMCHS_SD_SYSCTL_DTO_2POW27);

	/* Clean the MMCHS_SD_STAT register */
	mmc_write32(mmchs->regs->irpt, 0xffffffffu);
#ifdef USE_INTR
	hook_id = 1;
	if (sys_irqsetpolicy(mmchs->irq_nr, 0, &hook_id) != OK) {
		log_warn(&log, "mmc: couldn't set IRQ policy %d\n",
		    mmchs->irq_nr);
		return 1;
	}
	/* enable signaling from MMC controller towards interrupt controller */
	mmc_write32(mmchs->regs->irpt_en, 0xffffffffu);
#endif

	return 0;
}

void
intr_deassert(int mask)
{
	if (mmc_read32(mmchs->regs->irpt) & 0x8000) {
		log_warn(&log, "%s, error stat  %08x\n", __FUNCTION__,
		    mmc_read32(mmchs->regs->irpt));
		mmc_set32(mmchs->regs->irpt, MMCHS_SD_STAT_ERROR_MASK,
		    0xffffffffu);
	} else {
		mmc_write32(mmchs->regs->irpt, mask);
	}
}

/* pointer to the data to transfer used in bwr and brr */
unsigned char *io_data;
int io_len;

void
handle_bwr()
{
	/* handle buffer write ready interrupts. These happen in a non
	 * predictable way (eg. we send a request but don't know if we are
	 * first doing to get a request completed before we are allowed to
	 * send the data to the hardware or not */
	uint32_t value;
	uint32_t count;
	assert(mmc_read32(mmchs->regs->status) & MMCHS_SD_PSTATE_BWE_EN);
	assert(io_data != NULL);

	for (count = 0; count < io_len; count += 4) {
		while (!(mmc_read32(mmchs->regs->
			    status) & MMCHS_SD_PSTATE_BWE_EN)) {
			log_warn(&log,
			    "Error expected Buffer to be write enabled(%d)\n",
			    count);
		}
		*((char *) &value) = io_data[count];
		*((char *) &value + 1) = io_data[count + 1];
		*((char *) &value + 2) = io_data[count + 2];
		*((char *) &value + 3) = io_data[count + 3];
		mmc_write32(mmchs->regs->data, value);
	}
	intr_deassert(MMCHS_SD_IE_BWR_ENABLE);
	/* expect buffer to be write enabled */
	io_data = NULL;
}

void
handle_brr()
{
	/* handle buffer read ready interrupts. genrally these happen afther
	 * the data is read from the sd card. */

	uint32_t value;
	uint32_t count;

	/* Problem BRE should be true */
	assert(mmc_read32(mmchs->regs->status) & MMCHS_SD_PSTATE_BRE_EN);

	assert(io_data != NULL);

	for (count = 0; count < io_len; count += 4) {
		value = mmc_read32(mmchs->regs->data);
		io_data[count] = *((char *) &value);
		io_data[count + 1] = *((char *) &value + 1);
		io_data[count + 2] = *((char *) &value + 2);
		io_data[count + 3] = *((char *) &value + 3);
	}
	/* clear bbr interrupt */
	intr_deassert(MMCHS_SD_IE_BRR_ENABLE_ENABLE);
	io_data = NULL;
}

static void
mmchs_hw_intr(unsigned int irqs)
{
	log_warn(&log, "Hardware interrupt left over (0x%08lx)\n",
	    mmc_read32(mmchs->regs->irpt));

#ifdef USE_INTR
	if (sys_irqenable(&hook_id) != OK)
		printf("couldn't re-enable interrupt \n");
#endif
	/* Leftover interrupt(s) received; ack it/them. */
}

/*===========================================================================*
 *				w_intr_wait				     *
 *===========================================================================*/
static int
intr_wait(int mask)
{
	long v;
#ifdef USE_INTR
	if (sys_irqenable(&hook_id) != OK)
		printf("Failed to enable irqenable irq\n");
	/* Wait for a task completion interrupt. */
	message m;
	int ipc_status;
	int ticks = SANE_TIMEOUT * sys_hz() / 1000000;

	if (ticks <= 0)
		ticks = 1;
	while (1) {
		int rr;
		sys_setalarm(ticks, 0);
		if ((rr = driver_receive(ANY, &m, &ipc_status)) != OK) {
			panic("driver_receive failed: %d", rr);
		};
		if (is_ipc_notify(ipc_status)) {
			switch (_ENDPOINT_P(m.m_source)) {
			case CLOCK:
				/* Timeout. */
				log_warn(&log, "TIMEOUT\n");
				return 1;
				break;
			case HARDWARE:
				while ((v =
					mmc_read32(mmchs->regs->irpt)) !=
				    0) {
					if (v & MMCHS_SD_IE_BWR_ENABLE) {
						log_info(&log, "MMCHS_SD_IE_BWR_ENABLE 0x%x\n", v & MMCHS_SD_IE_BWR_ENABLE);
						handle_bwr();
						continue;
					}
					if (v & MMCHS_SD_IE_BRR_ENABLE) {
						log_info(&log, "MMCHS_SD_IE_BRR_ENABLE 0x%x\n", v & MMCHS_SD_IE_BRR_ENABLE);
						handle_brr();
						continue;
					}

					if (v & mask) {
						/* this is the normal return
						 * path, the mask given
						 * matches the pending
						 * interrupt. cancel the alarm
						 * and return */
						sys_setalarm(0, 0);
						return 0;
					} else if (v & (1 << 15)) {
						log_info(&log, "MASK 0x%x 0x%x\n", (1 << 15), v & (1 << 15));
						return 1;	/* error */
					}

					log_warn(&log,
					    "unexpected HW interrupt 0x%08x mask 0X%08x\n",
					    v, mask);
					if (sys_irqenable(&hook_id) != OK)
						printf
						    ("Failed to re-enable irqenable irq\n");
				}
				/* if we end up here re-enable interrupts for
				 * the next round */
				if (sys_irqenable(&hook_id) != OK)
					printf
					    ("Failed to re-enable irqenable irq\n");
				break;
			default:
				/* 
				 * unhandled notify message. Queue it and
				 * handle it in the blockdriver loop.
				 */
				blockdriver_mq_queue(&m, ipc_status);
			}
		} else {
			/* 
			 * unhandled message. Queue it and handle it in the
			 * blockdriver loop.
			 */
			blockdriver_mq_queue(&m, ipc_status);
		}
	}
	sys_setalarm(0, 0);	/* cancel the alarm */

#else
	spin_t spin;
	spin_init(&spin, SANE_TIMEOUT);
	/* Wait for completion */
	int counter = 0;
	while (1 == 1) {
		counter++;
		v = mmc_read32(mmchs->regs->irpt);
		if (spin_check(&spin) == FALSE) {
			log_warn(&log,
			    "Timeout waiting for interrupt (%d) value 0x%08x mask 0x%08x\n",
			    counter, v, mask);
			return 1;
		}
		if (v & MMCHS_SD_IE_BWR_ENABLE) {
			handle_bwr();
			continue;
		}
		if (v & MMCHS_SD_IE_BRR_ENABLE) {
			handle_brr();
			continue;
		}
		if (v & mask) {
			return 0;
		} else if (v & 0xFF00) {
			log_debug(&log,
			    "unexpected HW interrupt (%d) 0x%08x mask 0x%08x\n",
			    v, mask);
			return 1;
		}
	}
	return 1;		/* unreached */
#endif /* USE_INTR */
}

int
mmchs_send_cmd(uint32_t command, uint32_t arg)
{

	spin_t spin;
	/* Read current interrupt status and fail it an interrupt is already
	 * asserted */
	assert(mmc_read32(mmchs->regs->irpt) == 0);

	/* Set arguments */
	mmc_write32(mmchs->regs->arg1, arg);
	/* Set command */
	mmc_set32(mmchs->regs->cmdtm, MMCHS_SD_CMD_MASK, command);

	if (intr_wait(MMCHS_SD_STAT_CC)) {
		uint32_t v = mmc_read32(mmchs->regs->irpt);
		intr_deassert(MMCHS_SD_STAT_CC);
		log_warn(&log, "Failure waiting for interrupt 0x%lx\n", v);
		return 1;
	}
	intr_deassert(MMCHS_SD_STAT_DONE);

	if ((command & MMCHS_SD_CMD_RSP_TYPE) ==
	    MMCHS_SD_CMD_RSP_TYPE_48B_BUSY) {
		/* 
		 * Command with busy response *CAN* also set the TC bit if they exit busy
		 */
		if ((mmc_read32(mmchs->regs->irpt)
			& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
			log_warn(&log, "TC should be raised\n");
		}
		intr_deassert(MMCHS_SD_STAT_TC);
	}

	spin_init(&spin, SANE_TIMEOUT);
	while(1) {
		if(spin_check(&spin) == FALSE)
			break;
	}

	return 0;
}

int
mmc_send_cmd(struct mmc_command *c)
{

	/* convert the command to a hsmmc command */
	int ret;
	uint32_t cmd, arg;
	cmd = MMCHS_SD_CMD_INDX_CMD(c->cmd);
	arg = c->args;
	assert(c->data_type == DATA_NONE || c->data_type == DATA_READ
	    || c->data_type == DATA_WRITE);

	switch (c->resp_type) {
	case RESP_LEN_48_CHK_BUSY:
		cmd |= MMCHS_SD_CMD_RSP_TYPE_48B_BUSY;
		break;
	case RESP_LEN_48:
		cmd |= MMCHS_SD_CMD_RSP_TYPE_48B;
		break;
	case RESP_LEN_136:
		cmd |= MMCHS_SD_CMD_RSP_TYPE_136B;
		break;
	case RESP_NO_RESPONSE:
		cmd |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
		break;
	default:
		return 1;
	}

	/* read single block */
	if (c->data_type == DATA_READ) {
		cmd |= MMCHS_SD_CMD_DP_DATA;	/* Command with data transfer */
		cmd |= MMCHS_SD_CMD_MSBS_SINGLE;	/* single block */
		cmd |= MMCHS_SD_CMD_DDIR_READ;	/* read data from card */
	}

	/* write single block */
	if (c->data_type == DATA_WRITE) {
		cmd |= MMCHS_SD_CMD_DP_DATA;	/* Command with data transfer */
		cmd |= MMCHS_SD_CMD_MSBS_SINGLE;	/* single block */
		cmd |= MMCHS_SD_CMD_DDIR_WRITE;	/* write to the card */
	}

	/* check we are in a sane state */
	if ((mmc_read32(mmchs->regs->irpt) & 0xffffu)) {
		log_warn(&log, "%s, interrupt already raised stat  %08x\n",
		    __FUNCTION__, mmc_read32(mmchs->regs->irpt));
		mmc_write32(mmchs->regs->irpt, MMCHS_SD_IE_CC_ENABLE_CLEAR);
	}

	if (cmd & MMCHS_SD_CMD_DP_DATA) {
		if (cmd & MMCHS_SD_CMD_DDIR_READ) {
			/* if we are going to read enable the buffer ready
			 * interrupt */
			mmc_set32(mmchs->regs->irpt_mask,
			    MMCHS_SD_IE_BRR_ENABLE,
			    MMCHS_SD_IE_BRR_ENABLE_ENABLE);
		} else {
			mmc_set32(mmchs->regs->irpt_mask,
			    MMCHS_SD_IE_BWR_ENABLE,
			    MMCHS_SD_IE_BWR_ENABLE_ENABLE);
		}
		io_data = c->data;
		io_len = c->data_len;
		assert(io_len <= 0xFFF);	/* only 12 bits */
		assert(io_data != NULL);
		mmc_set32(mmchs->regs->blkscnt, MMCHS_SD_BLK_BLEN, io_len);
	}

	ret = mmchs_send_cmd(cmd, arg);

	if (cmd & MMCHS_SD_CMD_DP_DATA) {
		assert(c->data_len);
		if (cmd & MMCHS_SD_CMD_DDIR_READ) {
			/* Wait for TC */
			if (intr_wait(MMCHS_SD_IE_TC_ENABLE_ENABLE)) {
				intr_deassert(MMCHS_SD_IE_TC_ENABLE_ENABLE);
				log_warn(&log,
				    "(Read) Timeout waiting for interrupt\n");
				return 1;
			}

			mmc_write32(mmchs->regs->irpt,
			    MMCHS_SD_IE_TC_ENABLE_CLEAR);

			/* disable the bbr interrupt */
			mmc_set32(mmchs->regs->irpt_mask,
			    MMCHS_SD_IE_BRR_ENABLE,
			    MMCHS_SD_IE_BRR_ENABLE_DISABLE);
		} else {
			/* Wait for TC */
			if (intr_wait(MMCHS_SD_IE_TC_ENABLE_ENABLE)) {
				intr_deassert(MMCHS_SD_IE_TC_ENABLE_CLEAR);
				log_warn(&log,
				    "(Write) Timeout waiting for transfer complete\n");
				return 1;
			}
			intr_deassert(MMCHS_SD_IE_TC_ENABLE_CLEAR);

			mmc_set32(mmchs->regs->irpt_mask,
			    MMCHS_SD_IE_BWR_ENABLE,
			    MMCHS_SD_IE_BWR_ENABLE_DISABLE);

		}
	}

	/* copy response into cmd->resp */
	switch (c->resp_type) {
	case RESP_LEN_48_CHK_BUSY:
	case RESP_LEN_48:
		c->resp[0] = mmc_read32(mmchs->regs->resp0);
		break;
	case RESP_LEN_136:
		c->resp[0] = mmc_read32(mmchs->regs->resp0);
		c->resp[1] = mmc_read32(mmchs->regs->resp1);
		c->resp[2] = mmc_read32(mmchs->regs->resp2);
		c->resp[3] = mmc_read32(mmchs->regs->resp3);
		break;
	case RESP_NO_RESPONSE:
		break;
	default:
		return 1;
	}

	return ret;
}

int
mmc_send_app_cmd(struct sd_card_regs *card, struct mmc_command *c)
{
	struct mmc_command command;
	command.cmd = MMC_APP_CMD;
	command.resp_type = RESP_LEN_48;
	command.data_type = DATA_NONE;
	command.args = MMC_ARG_RCA(card->rca);
	if (mmc_send_cmd(&command)) {
		return 1;
	}
	return mmc_send_cmd(c);
}

int
card_goto_idle_state()
{
	struct mmc_command command;
	command.cmd = MMC_GO_IDLE_STATE;
	command.resp_type = RESP_NO_RESPONSE;
	command.data_type = DATA_NONE;
	command.args = 0x00;
	if (mmc_send_cmd(&command)) {
		// Failure
		return 1;
	}
	return 0;
}

int
card_identification()
{
	struct mmc_command command;
	command.cmd = SD_SEND_IF_COND;	/* Send CMD8 */
	command.resp_type = RESP_NO_RESPONSE;
	command.data_type = DATA_NONE;
	command.args = MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN;

	if (mmc_send_cmd(&command)) {
		/* We currently only support 2.0, and 1.0 won't respond to
		 * this request */
		log_warn(&log, "%s,  non SDHC card inserted\n", __FUNCTION__);
		return 1;
	}
	return 0;
}

int
card_query_voltage_and_type(struct sd_card_regs *card)
{
	struct mmc_command command;
	spin_t spin;

	command.cmd = SD_APP_OP_COND;
	command.resp_type = RESP_LEN_48;
	command.data_type = DATA_NONE;

	/* 0x1 << 30 == send HCS (Host capacity support) and get OCR register */
	command.args =
	    MMC_OCR_3_3V_3_4V | MMC_OCR_3_2V_3_3V | MMC_OCR_3_1V_3_2V |
	    MMC_OCR_3_0V_3_1V | MMC_OCR_2_9V_3_0V | MMC_OCR_2_8V_2_9V |
	    MMC_OCR_2_7V_2_8V;
	command.args |= MMC_OCR_HCS;	/* RCA=0000 */

	if (mmc_send_app_cmd(card, &command)) {
		return 1;
	}

	spin_init(&spin, SANE_TIMEOUT);
	while (!(command.resp[0] & MMC_OCR_MEM_READY)) {

		/* Send ACMD41 */
		/* 0x1 << 30 == send HCS (Host capacity support) and get OCR
		 * register */
		command.cmd = SD_APP_OP_COND;
		command.resp_type = RESP_LEN_48;
		command.data_type = DATA_NONE;
		/* 0x1 << 30 == send HCS (Host capacity support) */
		command.args = MMC_OCR_3_3V_3_4V | MMC_OCR_3_2V_3_3V
		    | MMC_OCR_3_1V_3_2V | MMC_OCR_3_0V_3_1V | MMC_OCR_2_9V_3_0V
		    | MMC_OCR_2_8V_2_9V | MMC_OCR_2_7V_2_8V;
		command.args |= MMC_OCR_HCS;	/* RCA=0000 */

		if (mmc_send_app_cmd(card, &command)) {
			return 1;
		}

		/* if bit 31 is set the response is valid */
		if ((command.resp[0] & MMC_OCR_MEM_READY)) {
			break;
		}
		if (spin_check(&spin) == FALSE) {
			log_warn(&log, "TIMEOUT waiting for the SD card\n");
		}

	}
	card->ocr = (command.resp[0] >> 8) & 0xffff;
	return 0;
}

int
card_identify(struct sd_card_regs *card)
{
	struct mmc_command command;
	/* Send cmd 2 (all_send_cid) and expect 136 bits response */
	command.cmd = MMC_ALL_SEND_CID;
	command.resp_type = RESP_LEN_136;
	command.data_type = DATA_NONE;
	command.args = MMC_ARG_RCA(0x0);	/* RCA=0000 */

	if (mmc_send_cmd(&command)) {
		return 1;
	}

	card->cid[0] = command.resp[0];
	card->cid[1] = command.resp[1];
	card->cid[2] = command.resp[2];
	card->cid[3] = command.resp[3];

	command.cmd = MMC_SET_RELATIVE_ADDR;
	command.resp_type = RESP_LEN_48;
	command.data_type = DATA_NONE;
	command.args = 0x0;	/* RCA=0000 */

	/* R6 response */
	if (mmc_send_cmd(&command)) {
		return 1;
	}

	card->rca = SD_R6_RCA(command.resp);
	/* MMHCS only supports a single card so sending MMCHS_SD_CMD_CMD2 is
	 * useless Still we should make it possible in the API to support
	 * multiple cards */

	return 0;
}

int
card_csd(struct sd_card_regs *card)
{
	/* Read the Card Specific Data register */
	struct mmc_command command;

	/* send_csd -> r2 response */
	command.cmd = MMC_SEND_CSD;
	command.resp_type = RESP_LEN_136;
	command.data_type = DATA_NONE;
	command.args = MMC_ARG_RCA(card->rca);	/* card rca */

	if (mmc_send_cmd(&command)) {
		return 1;
	}

	card->csd[0] = command.resp[0];
	card->csd[1] = command.resp[1];
	card->csd[2] = command.resp[2];
	card->csd[3] = command.resp[3];

	return 0;
}

int
select_card(struct sd_card_regs *card)
{
	struct mmc_command command;

	command.cmd = MMC_SELECT_CARD;
	command.resp_type = RESP_LEN_48_CHK_BUSY;
	command.data_type = DATA_NONE;
	command.args = MMC_ARG_RCA(card->rca);	/* card rca */

	if (mmc_send_cmd(&command)) {
		return 1;
	}
	return 0;
}

int
card_scr(struct sd_card_regs *card)
{
	uint8_t buffer[8];	/* 64 bits */
	uint8_t *p;
	int c;
	/* the SD CARD configuration register. This is an additional register
	 * next to the Card Specific register containing additional data we
	 * need */
	struct mmc_command command;

	log_trace(&log, "Read card scr\n");
	/* send_csd -> r2 response */
	command.cmd = SD_APP_SEND_SCR;
	command.resp_type = RESP_LEN_48;
	command.data_type = DATA_READ;
	command.args = 0;
	command.data = buffer;
	command.data_len = 8;

	if (mmc_send_app_cmd(card, &command)) {
		return 1;
	}

	p = (uint8_t *) card->scr;

	/* copy the data to card->scr */
	for (c = 7; c >= 0; c--) {
		*p++ = buffer[c];
	}

	if (!SCR_SD_BUS_WIDTHS(card->scr) & SCR_SD_BUS_WIDTHS_4BIT) {
		/* it would be very weird not to support 4 bits access */
		log_warn(&log, "4 bit access not supported\n");
	}

	log_trace(&log, "1 bit bus width %ssupported\n",
	    (SCR_SD_BUS_WIDTHS(card->scr) & SCR_SD_BUS_WIDTHS_1BIT) ? "" :
	    "un");
	log_trace(&log, "4 bit bus width %ssupported\n",
	    (SCR_SD_BUS_WIDTHS(card->scr) & SCR_SD_BUS_WIDTHS_4BIT) ? "" :
	    "un");

	return 0;
}

int
enable_4bit_mode(struct sd_card_regs *card)
{
	struct mmc_command command;

	if (SCR_SD_BUS_WIDTHS(card->scr) & SCR_SD_BUS_WIDTHS_4BIT) {
		/* set transfer width */
		command.cmd = SD_APP_SET_BUS_WIDTH;
		command.resp_type = RESP_LEN_48;
		command.data_type = DATA_NONE;
		command.args = 2;	/* 4 bits */

		if (mmc_send_app_cmd(card, &command)) {
			log_warn(&log,
			    "SD-card does not support 4 bit transfer\n");
			return 1;
		}
		/* now configure the controller to use 4 bit access */
		mmc_set32(mmchs->regs->control0, MMCHS_SD_HCTL_DTW,
		    MMCHS_SD_HCTL_DTW_4BIT);
		return 0;
	}
	return 1;		/* expect 4 bits mode to work so having a card
				 * that doesn't support 4 bits mode */
}

void
dump_char(char *out, char in)
{
	int i;
	memset(out, 0, 9);
	for (i = 0; i < 8; i++) {
		out[i] = ((in >> i) & 0x1) ? '1' : '0';
	}

}

void
dump(uint8_t * data, int len)
{
	int c;
	char digit[4][9];
	char *p = data;

	for (c = 0; c < len;) {
		memset(digit, 0, sizeof(digit));
		if (c++ < len)
			dump_char(digit[0], *data++);
		if (c++ < len)
			dump_char(digit[1], *data++);
		if (c++ < len)
			dump_char(digit[2], *data++);
		if (c++ < len)
			dump_char(digit[3], *data++);
		printf("%x %s %s %s %s\n", c, digit[0], digit[1], digit[2],
		    digit[3]);
	}
}

void
mmc_switch(int function, int value, uint8_t * data)
{
	struct mmc_command command;

	/* function index */
	int findex, fshift;
	findex = function - 1;
	fshift = findex << 2;	/* bits used per function */

	command.cmd = MMC_SWITCH;
	command.resp_type = RESP_LEN_48;
	command.data_type = DATA_READ;
	command.data = data;
	command.data_len = 64;
	command.args = (1 << 31) | (0x00ffffff & ~(0xf << fshift));
	command.args |= (value << fshift);
	if (mmc_send_cmd(&command)) {
		log_warn(&log, "Failed to set device in high speed mode\n");
		return;
	}
	// dump(data,64);
}

int
read_single_block(struct sd_card *card, uint32_t blknr, unsigned char *buf)
{
	struct mmc_command command;

	command.cmd = MMC_READ_BLOCK_SINGLE;
	command.args = blknr;
	command.resp_type = RESP_LEN_48;
	command.data_type = DATA_READ;
	command.data = buf;
	command.data_len = card->blk_size;

	if (mmc_send_cmd(&command)) {
		log_warn(&log, "Error sending command\n");
		return 1;
	}

	return 0;
}

int
write_single_block(struct sd_card *card,
    uint32_t blknr, unsigned char *buf)
{
	struct mmc_command command;

	command.cmd = MMC_WRITE_BLOCK_SINGLE;
	command.args = blknr;
	command.resp_type = RESP_LEN_48;
	command.data_type = DATA_WRITE;
	command.data = buf;
	command.data_len = card->blk_size;

	/* write single block */
	if (mmc_send_cmd(&command)) {
		log_warn(&log, "Write single block command failed\n");
		return 1;
	}

	return 0;
}

int
mmchs_host_init(struct mmc_host *host)
{
	mmchs_init(1);
	return 0;
}

int
mmchs_host_set_instance(struct mmc_host *host, int instance)
{
	log_info(&log, "Using instance number %d\n", instance);
	if (instance != 0) {
		return EIO;
	}
	return OK;
}

int
mmchs_host_reset(struct mmc_host *host)
{
	mmchs_init(1);
	return 0;
}

int
mmchs_card_detect(struct sd_slot *slot)
{
	/* @TODO implement proper card detect */
	return 1;
}

struct sd_card *
mmchs_card_initialize(struct sd_slot *slot)
{
	int blksize;
	struct sd_card *card;

	mmchs_init(1);

	card = &slot->card;
	memset(card, 0, sizeof(struct sd_card));
	card->slot = slot;

	if (card_goto_idle_state()) {
		log_warn(&log, "Failed to go idle state\n");
		return NULL;
	}

	if (card_identification()) {
		log_warn(&log, "Failed to do card_identification\n");
		return NULL;
	}

	if (card_query_voltage_and_type(&slot->card.regs)) {
		log_warn(&log, "Failed to do card_query_voltage_and_type\n");
		return NULL;
	}

	if (card_identify(&slot->card.regs)) {
		log_warn(&log, "Failed to identify card\n");
		return NULL;
	}
	/* We have now initialized the hardware identified the card */
	if (card_csd(&slot->card.regs)) {
		log_warn(&log, "failed to read csd (card specific data)\n");
		return NULL;
	}

	if (select_card(&slot->card.regs)) {
		log_warn(&log, "Failed to select card\n");
		return NULL;
	}

	if (card_scr(&slot->card.regs)) {
		log_warn(&log,
		    "failed to read scr (card additional specific data)\n");
		return NULL;
	}

	if (enable_4bit_mode(&slot->card.regs)) {
		log_warn(&log, "failed to configure 4 bit access mode\n");
		return NULL;
	}

	switch (SD_CSD_READ_BL_LEN(slot->card.regs.csd)) {
		case 0x09:
			blksize = 512;
			break;
		case 0x0a:
			blksize = 1024;
			break;
		case 0x0b:
			blksize = 2048;
			break;
		default:
			log_warn(&log, "Unknown block size\n");
			return NULL;
	}
	slot->card.blk_size = blksize;
	
	int version;
	switch (SD_CSD_CSDVER(slot->card.regs.csd)) {
		case 0:
			version = 1;
			break;
		case 1:
			version = 2;
			break;
	}
	if (version == 1)
		slot->card.blk_count = SD_CSD_CAPACITY(slot->card.regs.csd);
	else
		slot->card.blk_count = SD_CSD_V2_CAPACITY(slot->card.regs.csd);
	slot->card.state = SD_MODE_DATA_TRANSFER_MODE;

	/* MINIX related stuff to keep track of partitions */
	memset(slot->card.part, 0, sizeof(slot->card.part));
	memset(slot->card.subpart, 0, sizeof(slot->card.subpart));
	slot->card.part[0].dv_base = 0;
	if (version == 1)
		slot->card.part[0].dv_size =
	    	(unsigned long long) SD_CSD_CAPACITY(slot->card.regs.csd) * blksize;
	else 
		slot->card.part[0].dv_size =
	    	(unsigned long long) SD_CSD_V2_CAPACITY(slot->card.regs.csd) * blksize;

	log_debug(&log, "CARD: blk_size %ld blk_count %ld\n", slot->card.blk_size, slot->card.blk_count);
	log_debug(&log, "part0: dv_base %lld dv_size %lld\n", slot->card.part[0].dv_base, slot->card.part[0].dv_size);
	log_debug(&log, "part1: dv_base %lld dv_size %lld\n", slot->card.part[1].dv_base, slot->card.part[1].dv_size);
	log_debug(&log, "part2: dv_base %lld dv_size %lld\n", slot->card.part[2].dv_base, slot->card.part[2].dv_size);
	log_debug(&log, "part3: dv_base %lld dv_size %lld\n", slot->card.part[3].dv_base, slot->card.part[3].dv_size);
	return &slot->card;
}

/* read count blocks into existing buf */
static int
mmchs_host_read(struct sd_card *card,
    uint32_t blknr, uint32_t count, unsigned char *buf)
{
	for (uint32_t i = 0; i < count; i++) {
		log_info(&log, "read %d block nr %p blk_size %d count %d\n", blknr + i, buf + (i * card->blk_size), card->blk_size, count);
		read_single_block(card, blknr + i,
		    buf + (i * card->blk_size));
	}
	return OK;
}

/* write count blocks */
static int
mmchs_host_write(struct sd_card *card,
    uint32_t blknr, uint32_t count, unsigned char *buf)
{
	uint32_t i;

	i = count;
	for (i = 0; i < count; i++) {
		log_info(&log, "write %d block nr\n", blknr + i);
		write_single_block(card, blknr + i,
		    buf + (i * card->blk_size));
	}

	return OK;
}

int
mmchs_card_release(struct sd_card *card)
{
	assert(card->open_ct == 1);
	card->open_ct--;
	card->state = SD_MODE_UNINITIALIZED;
	/* TODO:Set card state */

	/* now configure the controller to use 4 bit access */
	mmc_set32(mmchs->regs->control0, MMCHS_SD_HCTL_DTW,
	    MMCHS_SD_HCTL_DTW_1BIT);

	return OK;
}

void
host_initialize_host_structure_mmchs(struct mmc_host *host)
{
	/* Initialize the basic data structures host slots and cards */
	int i;
	mmchs = NULL;

	struct machine  machine ;
	sys_getmachine(&machine);

	mmchs = &rpi_sdcard;
	
	assert(mmchs);
	host->host_set_instance = mmchs_host_set_instance;
	host->host_init = mmchs_host_init;
	host->host_reset = mmchs_host_reset;
	host->card_detect = mmchs_card_detect;
	host->card_initialize = mmchs_card_initialize;
	host->card_release = mmchs_card_release;
	host->hw_intr = mmchs_hw_intr;
	host->read = mmchs_host_read;
	host->write = mmchs_host_write;

	/* initialize data structures */
	for (i = 0; i < sizeof(host->slot) / sizeof(host->slot[0]); i++) {
		// @TODO set initial card and slot state
		host->slot[i].host = host;
		host->slot[i].card.slot = &host->slot[i];
	}

	/* Customize name for logs */
	log.name = "mmc_rpi";
}
