// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <cpu_func.h>
#include <linux/string.h>
#include <linux/arm-smccc.h>
#include <linux/types.h>
#include <env.h>

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
