/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	printk("Zephyr Example Application %s\n", APP_VERSION_STRING);
	int ret;
	const struct device *sensor;


	sensor = DEVICE_DT_GET(DT_NODELABEL(mt6701));
	if (!device_is_ready(sensor)) {
		LOG_ERR("Sensor not ready");
		return 0;
	}

	while (1) {
		struct sensor_value val;

		ret = sensor_sample_fetch(sensor);
		if (ret < 0) {
			LOG_ERR("Could not fetch sample (%d)", ret);
			return 0;
		}

		ret = sensor_channel_get(sensor, SENSOR_CHAN_ROTATION, &val);
		if (ret < 0) {
			LOG_ERR("Could not get sample (%d)", ret);
			return 0;
		}

		printk("Rotation value: %d.%06d\n", val.val1, val.val2);

    k_msleep(50);
	}

	return 0;
}

