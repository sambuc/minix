#ifndef __ACPI_H__
#define __ACPI_H__

#include "kernel/kernel.h"

/* ACPI root system description pointer */
struct acpi_rsdp {
	char	signature[8]; /* must be "RSD PTR " */
	uint8_t	checksum;
	char	oemid[6];
	uint8_t	revision;
	u32_t	rsdt_addr;
	u32_t	length;
}; 

#define ACPI_SDT_SIGNATURE_LEN	4

#define ACPI_SDT_SIGNATURE(name)	#name

/* header common to all system description tables */
struct acpi_sdt_header {
	char	signature[ACPI_SDT_SIGNATURE_LEN];
	u32_t	length;
	uint8_t	revision;
	uint8_t	checksum;
	char	oemid[6];
	char	oem_table_id[8];
	u32_t	oem_revision;
	u32_t	creator_id;
	u32_t	creator_revision;
};

struct acpi_generic_address {
	uint8_t address_space_id;
	uint8_t register_bit_width;
	uint8_t register_bit_offset;
	uint8_t access_size;
	u64_t address;
};

struct acpi_fadt_header
{
	struct acpi_sdt_header hdr;
	u32_t facs;
	u32_t dsdt;
	uint8_t model;
	uint8_t preferred_pm_profile;
	u16_t sci_int;
	u32_t smi_cmd;
	uint8_t acpi_enable;
	uint8_t acpi_disable;
	uint8_t s4bios_req;
	uint8_t pstate_cnt;
	u32_t pm1a_evt_blk;
	u32_t pm1b_evt_blk;
	u32_t pm1a_cnt_blk;
	u32_t pm1b_cnt_blk;
	u32_t pm2_cnt_blk;
	u32_t pm_tmr_blk;
	u32_t gpe0_blk;
	u32_t gpe1_blk;
	uint8_t pm1_evt_len;
	uint8_t pm1_cnt_len;
	uint8_t pm2_cnt_len;
	uint8_t pm_tmr_len;
	uint8_t gpe0_blk_len;
	uint8_t gpe1_blk_len;
	uint8_t gpe1_base;
	uint8_t cst_cnt;
	u16_t p_lvl2_lat;
	u16_t p_lvl3_lat;
	u16_t flush_size;
	u16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alrm;
	uint8_t mon_alrm;
	uint8_t century;
	u16_t iapc_boot_arch;
	uint8_t reserved1;
	u32_t flags;
	struct acpi_generic_address reset_reg;
	uint8_t reset_value;
	uint8_t reserved2[3];
	u64_t xfacs;
	u64_t xdsdt;
	struct acpi_generic_address xpm1a_evt_blk;
	struct acpi_generic_address xpm1b_evt_blk;
	struct acpi_generic_address xpm1a_cnt_blk;
	struct acpi_generic_address xpm1b_cnt_blk;
	struct acpi_generic_address xpm2_cnt_blk;
	struct acpi_generic_address xpm_tmr_blk;
	struct acpi_generic_address xgpe0_blk;
	struct acpi_generic_address xgpe1_blk;
};

struct acpi_madt_hdr {
	struct acpi_sdt_header	hdr;
	u32_t			local_apic_address;
	u32_t			flags;
};

#define ACPI_MADT_TYPE_LAPIC		0
#define ACPI_MADT_TYPE_IOAPIC		1
#define ACPI_MADT_TYPE_INT_SRC		2
#define ACPI_MADT_TYPE_NMI_SRC		3
#define ACPI_MADT_TYPE_LAPIC_NMI	4
#define ACPI_MADT_TYPE_LAPIC_ADRESS	5
#define ACPI_MADT_TYPE_IOSAPIC		6
#define ACPI_MADT_TYPE_LSAPIC		7
#define ACPI_MADT_TYPE_PLATFORM_INT_SRC	8
#define ACPI_MADT_TYPE_Lx2APIC		9
#define ACPI_MADT_TYPE_Lx2APIC_NMI	10

struct acpi_madt_item_hdr{
	uint8_t	type;
	uint8_t	length;
};

struct acpi_madt_lapic {
	struct acpi_madt_item_hdr hdr;
	uint8_t	acpi_cpu_id;
	uint8_t	apic_id;
	u32_t	flags;
};

struct acpi_madt_ioapic {
	struct acpi_madt_item_hdr hdr;
	uint8_t	id;
	uint8_t	__reserved;
	u32_t	address;
	u32_t	global_int_base;
};

struct acpi_madt_int_src {
	struct acpi_madt_item_hdr hdr;
	uint8_t	bus;
	uint8_t	bus_int;
	u32_t	global_int;
	u16_t	mps_flags;
};

struct acpi_madt_nmi {
	struct acpi_madt_item_hdr hdr;
	u16_t	flags;
	u32_t	global_int;
};

void acpi_init(void);

void acpi_poweroff(void);

/* 
 * Returns a pointer to the io acpi structure in the MADT table in ACPI. The
 * pointer is valid only until paging is turned off. No memory is allocated in
 * this function thus no memory needs to be freed
 */
struct acpi_madt_ioapic * acpi_get_ioapic_next(void);
/* same as above for local APICs */
struct acpi_madt_lapic * acpi_get_lapic_next(void);

#endif /* __ACPI_H__ */
