#include <asm-generic/gpio.h>
#include <sunxi_gpadc.h>
#include <dm/device.h>

#define countof(x) (sizeof(x) / sizeof(x[0]))
#define HW_ID_ADC_CHANNEL	2
/*PK 24 */
#define HW_ID_GPIO			344

struct hw_info_def {
	char *compatible;
	char *fdtfile;
	unsigned int hw_id_level;
	unsigned int hw_id_lower_bound;
	unsigned int hw_id_upper_bound;
};

static struct hw_info_def hw_info[] = {
	{"allwinner,a733", "allwinner/sun60i-a733-cubie-a7s.dtb", 1, 1265, 1365}, //1315mv +/- 50mv
	{"allwinner,a733", "allwinner/sun60i-a733-cubie-a7a.dtb", 1, 1519, 1619}, //1569mv +/- 50mv
	{"allwinner,a733", "allwinner/sun60i-a733-cubie-a7z.dtb", 1, 1750, 1850}, //1800mv +/- 50mv
};
void radxa_set_compat_fdt(void)
{
	int i, vol, level;

	if (gpio_request(HW_ID_GPIO, "hw_id_level") != 0) {
		debug("Failed to request GPIO %d for hardware ID detection\n", HW_ID_GPIO);
		return;
	}
	gpio_direction_input(HW_ID_GPIO);
	level = gpio_get_value(HW_ID_GPIO);
	vol = sunxi_get_gpadc_vol(HW_ID_ADC_CHANNEL);
	gpio_free(HW_ID_GPIO);

	debug("%s vol=%d, level=%d\n", __func__, vol, level);
	for (i = 0; i < countof(hw_info); i++) {
		if (of_machine_is_compatible(hw_info[i].compatible) &&
			vol >= hw_info[i].hw_id_lower_bound &&
			vol <= hw_info[i].hw_id_upper_bound &&
			level == hw_info[i].hw_id_level) {
			env_set("fdtfile", hw_info[i].fdtfile);
			printf("Override default fdtfile to %s\n", hw_info[i].fdtfile);
			return;
		}
	}
	printf("No compatible fdtfile found for vol=%d, level=%d\n", vol, level);
}
