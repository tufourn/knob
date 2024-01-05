#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(encoder, CONFIG_SENSOR_LOG_LEVEL);

#define ENCODER_POLLING_INTERVAL_MS 10 

static uint32_t rotation_int = 0;
static uint32_t rotation_frac = 0;

const struct device *encoder = DEVICE_DT_GET(DT_NODELABEL(mt6701));

int encoder_get_rotation() {
  return rotation_int;
}

void encoder_work_handler(struct k_work *work) {
  int ret;
  struct sensor_value val;

  ret = sensor_sample_fetch(encoder);
  if (ret < 0) {
    LOG_ERR("Could not fetch sample (%d)", ret);
  }

  ret = sensor_channel_get(encoder, SENSOR_CHAN_ROTATION, &val);
  if (ret < 0) {
    LOG_ERR("Could not get sample (%d)", ret);
  }

  rotation_int = val.val1;
  rotation_frac = val.val2;

  LOG_INF("Rotation value: %d.%06d", rotation_int, rotation_frac);
}

K_WORK_DEFINE(encoder_work, encoder_work_handler);

void encoder_expiry_function() {
  k_work_submit(&encoder_work);
}

K_TIMER_DEFINE(encoder_timer, encoder_expiry_function, NULL);

int encoder_init() {
  k_timer_start(&encoder_timer,
                K_MSEC(ENCODER_POLLING_INTERVAL_MS),
                K_MSEC(ENCODER_POLLING_INTERVAL_MS));
  return 0;
}

SYS_INIT(encoder_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
