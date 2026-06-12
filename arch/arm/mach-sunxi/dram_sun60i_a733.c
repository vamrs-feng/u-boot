#include <asm/arch/spl.h>
#include <linux/sizes.h>
#include <sunxi_image.h>

unsigned long sunxi_dram_init(void)
{
	struct boot_file_head *spl = (void *)(ulong)SPL_ADDR;
	u32 size = spl->dram_size;

	return (unsigned long)size * SZ_1M;
}
