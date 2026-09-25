#include <zephyr/logging/log.h>

#include "update.h"

LOG_MODULE_REGISTER(app1);

int main(void)
{
	LOG_INF("*** APP1 ***");
	update_run();
	return 0;
}
