#include "zephyr/device.h"
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(disp, CONFIG_APP_LOG_LEVEL);

static const struct device *display = DEVICE_DT_GET(DT_NODELABEL(ssd1306));

static void write_display(int val) {
  int ret;
  char buffer[15];
  snprintf(buffer, sizeof(buffer), "%03u degrees", val);
  ret = cfb_print(display, buffer, 0, 0);
  if (ret != 0) {
    LOG_ERR("failed to print %d", ret);
  }
  ret = cfb_framebuffer_finalize(display);
  if (ret != 0) {
    LOG_ERR("failed to finalize %d", ret);
  }
}

static void input_cb(struct input_event *evt) {
  if (evt->code == INPUT_ABS_Z) {
    write_display(evt->value);
  }
}

int display_init() {
  int ret;
  if (!device_is_ready(display)) {
    LOG_ERR("device not ready");
  }
  ret = cfb_framebuffer_init(display);
  if (ret != 0) {
    LOG_ERR("failed to init %d", ret);
    return ret;
  }
  ret = cfb_framebuffer_invert(display);
  if (ret != 0) {
    LOG_ERR("failed to invert %d", ret);
    return ret;
  }
  return 0;
}

INPUT_CALLBACK_DEFINE(NULL, input_cb);

SYS_INIT(display_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
