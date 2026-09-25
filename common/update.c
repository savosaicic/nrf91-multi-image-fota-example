#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <dk_buttons_and_leds.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>

#include "update.h"

LOG_MODULE_REGISTER(update);

static const char *const files[] = {CONFIG_UPDATE_FILE_APP0,
                                    CONFIG_UPDATE_FILE_APP1};

static K_SEM_DEFINE(button_sem, 0, 1);
static K_SEM_DEFINE(fota_sem, 0, 1);
static atomic_t target;
static bool fota_ok;

static void button_handler(uint32_t state, uint32_t changed)
{
	uint32_t pressed = state & changed;

	if (pressed & (DK_BTN1_MSK | DK_BTN2_MSK)) {
		atomic_set(&target, (pressed & DK_BTN1_MSK) ? 0 : 1);
		k_sem_give(&button_sem);
	}
}

static void fota_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		LOG_INF("progress: %d%%", evt->progress);
		break;
	case FOTA_DOWNLOAD_EVT_FINISHED:
		fota_ok = true;
		k_sem_give(&fota_sem);
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
	case FOTA_DOWNLOAD_EVT_CANCELLED:
		fota_ok = false;
		k_sem_give(&fota_sem);
		break;
	default:
		break;
	}
}

static int update(int img)
{
#if CONFIG_UPDATE_SEC_TAG >= 0
	static const int sec_tag = CONFIG_UPDATE_SEC_TAG;
#endif
	const struct fota_download_params params = {
		.host = CONFIG_UPDATE_HOST,
		.file = files[img],
#if CONFIG_UPDATE_SEC_TAG >= 0
		.sec_tag_list = &sec_tag,
		.sec_tag_count = 1,
#endif
		.expected_type = DFU_TARGET_IMAGE_TYPE_MCUBOOT,
		.img_num = img,
	};
	int err;

	LOG_INF("updating app%d from %s/%s", img, params.host, params.file);

	err = fota_download_start_params(&params);
	if (err) {
		return err;
	}

	k_sem_take(&fota_sem, K_FOREVER);
	return fota_ok ? 0 : -EIO;
}

void update_run(void)
{
	int err;

	if (!boot_is_img_confirmed()) {
		boot_write_img_confirmed();
		LOG_INF("image marked as confirmed");
	}

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("modem_lib_init failed: %d", err);
		return;
	}

	err = fota_download_init(fota_handler);
	if (err) {
		LOG_ERR("fota_download_init failed: %d", err);
		return;
	}

	LOG_INF("connecting to LTE...");
	err = lte_lc_connect();
	if (err) {
		LOG_ERR("lte_lc_connect failed: %d", err);
		return;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init failed: %d", err);
		return;
	}

	while (true) {
		LOG_INF("BTN1: update app0, BTN2: update app1");
		k_sem_reset(&button_sem);
		k_sem_take(&button_sem, K_FOREVER);

		err = update(atomic_get(&target));
		if (err) {
			LOG_ERR("update failed: %d", err);
			continue;
		}

		LOG_INF("download done, rebooting...");
		LOG_PANIC();
		sys_reboot(SYS_REBOOT_COLD);
	}
}
