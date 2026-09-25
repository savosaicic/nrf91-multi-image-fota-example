#include <zephyr/logging/log.h>

#include "update.h"

LOG_MODULE_REGISTER(app0);

int main(void)
{
	LOG_INF("*** APP0 ***");
	update_run();
	return 0;
}
