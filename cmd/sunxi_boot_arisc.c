// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <cpu_func.h>
#include <linux/string.h>
#include <linux/arm-smccc.h>
#include <linux/types.h>

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

	sunxi_smc_call_atf(ARM_SVC_ARISC_STARTUP, 0, 0, 0, 0);

	return 0;
}

U_BOOT_CMD(
	sunxi_boot_arisc, CONFIG_SYS_MAXARGS, 1, do_sunxi_boot_arisc,
	"Load and run arisc cpu",
	"\n"
);
