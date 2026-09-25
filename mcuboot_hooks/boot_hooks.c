/*
 * MCUboot image selection hook.
 *
 *   1. An image with a pending swap is booted.
 *   2. Otherwise app1 if BTN2 is held at reset, else app0.
 *   3. If the selected image is not bootable, boot the other one.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>

#include "bootutil/bootutil.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/fault_injection_hardening.h"

/* For mcuboot workaround */
#include <flash_map_backend/flash_map_backend.h>
#include "bootutil_priv.h"

static const struct gpio_dt_spec btn2 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

/*
 * WORKAROUND(mcuboot#2848): boot/zephyr/flash_check.c dereferences
 * state->imgs[0][slot].area, but boot_go_for_image_id(rsp, 1) only opens
 * image 1's areas. Pre open image 0's areas before booting image 1
 */
static int open_image0_areas(void)
{
	struct boot_loader_state *state = boot_get_loader_state();
	const uint8_t slots[] = {BOOT_SLOT_PRIMARY, BOOT_SLOT_SECONDARY};

	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		const struct flash_area **area = &state->imgs[0][slots[i]].area;

		if (*area == NULL &&
		    flash_area_open(flash_area_id_from_multi_image_slot(0, slots[i]),
                        area) != 0) {
			return -1;
		}
	}

	return 0;
}

static fih_ret boot_image(struct boot_rsp *rsp, int image)
{
	FIH_DECLARE(fih_rc, FIH_FAILURE);

	printk("boot_go_hook: booting app%d\n", image);

	if (image != 0 && open_image0_areas() != 0) {
		FIH_RET(FIH_FAILURE);
	}

	FIH_CALL(boot_go_for_image_id, fih_rc, rsp, (uint32_t)image);
	FIH_RET(fih_rc);
}

static int select_image(void)
{
	for (int i = 0; i < 2; i++) {
		int swap = boot_swap_type_multi(i);

		if (swap == BOOT_SWAP_TYPE_TEST || swap == BOOT_SWAP_TYPE_PERM ||
		    swap == BOOT_SWAP_TYPE_REVERT) {
			printk("boot_go_hook: pending swap on app%d\n", i);
			return i;
		}
	}

	if (gpio_is_ready_dt(&btn2) && gpio_pin_configure_dt(&btn2, GPIO_INPUT) == 0) {
		k_msleep(10);
		if (gpio_pin_get_dt(&btn2) > 0) {
			printk("boot_go_hook: BTN2 held\n");
			return 1;
		}
	}

	return 0;
}

fih_ret boot_go_hook(struct boot_rsp *rsp)
{
	FIH_DECLARE(fih_rc, FIH_FAILURE);
	int image = select_image();

	FIH_CALL(boot_image, fih_rc, rsp, image);
	if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
		printk("boot_go_hook: app%d not bootable\n", image);
		FIH_CALL(boot_image, fih_rc, rsp, !image);
	}

	FIH_RET(fih_rc);
}
