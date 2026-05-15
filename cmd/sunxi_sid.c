// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Radxa
 * sunxi SID (Security ID) command
 */

#include <command.h>
#include <asm/arch/cpu.h>

static int do_sunxi_sid(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	unsigned int chipid[4];
	unsigned int serial[4];
	int ret;

	ret = sunxi_get_sid(chipid);
	if (ret) {
		printf("Failed to read chipid: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("chipid  : %08x %08x %08x %08x\n",
	       chipid[0], chipid[1], chipid[2], chipid[3]);

	ret = sunxi_get_serial(serial);
	if (ret) {
		printf("Failed to read serial: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("serial  : %08x%08x%08x%08x\n",
	       serial[0], serial[1], serial[2], serial[3]);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	sid, 1, 0, do_sunxi_sid,
	"display sunxi chipid and serial",
	""
);
