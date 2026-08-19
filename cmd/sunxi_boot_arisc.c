// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <cpu_func.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/libfdt.h>
#include <linux/string.h>
#include <linux/arm-smccc.h>
#include <linux/types.h>
#include <env.h>

#ifdef CONFIG_MACH_SUN60I_A733
/*
 * Temporary bring-up interface for the A733 ddr.bin with SHA-256
 * d89804f404f2eef6ef5b03d82a310ef0906c4db8ded760be221b131f5c813a05.
 * The blob keeps its 96-word trained DRAM parameter block in SRAM.  Keep all
 * layout checks here so a different blob fails closed instead of passing
 * arbitrary data to ARISC.
 */
#define A733_LIBDRAM_GETTER_OFFSET	0x3e0
#define A733_LIBDRAM_GETTER_CODE		0x47704800
#define A733_LIBDRAM_PARA_PTR_OFFSET	0x3e4
#define A733_LIBDRAM_PARA_OFFSET		0x38fe0
#define A733_LIBDRAM_BSS_START_OFFSET	0x54
#define A733_LIBDRAM_BSS_END_OFFSET	0x58
#define A733_LIBDRAM_BSS_START		0x000c8bdc
#define A733_LIBDRAM_BSS_END		0x000c94a4
#define A733_DRAM_PARA_WORDS		96
#define A733_DRAM_FDT_EXTRA_SIZE		4096

static int sunxi_a733_inject_dram_para(void *fdt)
{
	uintptr_t blob = CONFIG_SUNXI_LIBDRAM_BASE;
	u32 para[A733_DRAM_PARA_WORDS];
	u32 getter_code, para_addr, bss_start, bss_end;
	char prop[16];
	int node, ret, i;

	getter_code = readl((void *)(blob + A733_LIBDRAM_GETTER_OFFSET));
	para_addr = readl((void *)(blob + A733_LIBDRAM_PARA_PTR_OFFSET));
	bss_start = readl((void *)(blob + A733_LIBDRAM_BSS_START_OFFSET));
	bss_end = readl((void *)(blob + A733_LIBDRAM_BSS_END_OFFSET));

	if (getter_code != A733_LIBDRAM_GETTER_CODE ||
	    para_addr != blob + A733_LIBDRAM_PARA_OFFSET ||
	    bss_start != A733_LIBDRAM_BSS_START ||
	    bss_end != A733_LIBDRAM_BSS_END ||
	    para_addr + sizeof(para) > bss_end) {
		printf("[SCP] ERROR: unsupported A733 ddr.bin layout\n");
		return -EINVAL;
	}

	for (i = 0; i < A733_DRAM_PARA_WORDS; i++)
		para[i] = readl((void *)(uintptr_t)(para_addr + i * sizeof(u32)));

	if (para[0] < 400 || para[0] > 4000 ||
	    (para[1] != 8 && para[1] != 9) ||
	    !para[6] || !para[7] || !para[30]) {
		printf("[SCP] ERROR: invalid trained DRAM parameters: "
		       "clk=%x type=%x para1=%x para2=%x tpr13=%x\n",
		       para[0], para[1], para[6], para[7], para[30]);
		return -EINVAL;
	}

	ret = fdt_check_header(fdt);
	if (ret)
		return ret;

	ret = fdt_increase_size(fdt, A733_DRAM_FDT_EXTRA_SIZE);
	if (ret) {
		printf("[SCP] ERROR: failed to grow FDT: %s\n",
		       fdt_strerror(ret));
		return ret;
	}

	node = fdt_path_offset(fdt, "/dram");
	if (node < 0) {
		printf("[SCP] ERROR: no /dram node: %s\n", fdt_strerror(node));
		return node;
	}

	for (i = 0; i < A733_DRAM_PARA_WORDS; i++) {
		sprintf(prop, "dram_para%02d", i);
		ret = fdt_setprop_u32(fdt, node, prop, para[i]);
		if (ret) {
			printf("[SCP] ERROR: failed to set %s: %s\n",
			       prop, fdt_strerror(ret));
			return ret;
		}
	}

	printf("[SCP] INFO: injected trained DRAM parameters: "
	       "clk=%x type=%x para1=%x para2=%x tpr13=%x\n",
	       para[0], para[1], para[6], para[7], para[30]);
	flush_cache((ulong)fdt, fdt_totalsize(fdt));

	return 0;
}
#endif

static u32 sunxi_smc_call_atf(ulong arg0, ulong arg1, ulong arg2, ulong arg3, ulong pResult)
{
	struct arm_smccc_res param = { 0 };

	arm_smccc_smc(arg0, arg1, arg2, arg3, 0, 0, 0, 0, &param);

	return param.a0;
}

static int do_sunxi_boot_arisc(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[])
{
	u32 ARM_SVC_ARISC_STARTUP = 0x8000ff10;
	unsigned long dtb_addr;

	printf("[SCP] INFO: starting arisc cpu...\n");

	dtb_addr = env_get_hex("fdt_addr_r", 0x1234);

	if (dtb_addr == 0x1234) {
		printf("[SCP] WARNING: fdt_addr_r is not set, power management may malfunction\n");
		dtb_addr = 0;
	} else {
		printf("[SCP] INFO: fdt_addr_r = 0x%08lx\n", dtb_addr);
		if (dtb_addr) {
			u32 magic = *(volatile u32 *)(uintptr_t)dtb_addr;
			if (magic == 0xedfe0dd0) {
				printf("[SCP] INFO: dtb magic verified at 0x%08lx\n", dtb_addr);
			} else {
				printf("[SCP] WARNING: dtb magic mismatch, expected 0xedfe0dd0 got 0x%08x, power management may malfunction\n", magic);
			}

#ifdef CONFIG_MACH_SUN60I_A733
			if (sunxi_a733_inject_dram_para((void *)(uintptr_t)dtb_addr)) {
				printf("[SCP] ERROR: refusing to start ARISC without trained DRAM parameters\n");
				return CMD_RET_FAILURE;
			}
#endif
		}
	}

	sunxi_smc_call_atf(ARM_SVC_ARISC_STARTUP, (ulong)dtb_addr, 0, 0, 0);

	return 0;
}

U_BOOT_CMD(
	sunxi_boot_arisc, CONFIG_SYS_MAXARGS, 1, do_sunxi_boot_arisc,
	"Load and run arisc cpu",
	"\n"
);
