#define DT_DRV_COMPAT magntek_mt6701

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include "zephyr/devicetree.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mt6701, CONFIG_INPUT_LOG_LEVEL);

struct mt6701_data {

};

struct mt6701_config {
  const struct spi_dt_spec bus;
};

static int mt6701_read(const struct device *dev, uint32_t *buf) {
  uint8_t rx_data[3];
  const struct mt6701_config *config = dev->config;
  const struct spi_buf rx_buf[1] = {
    {
    .buf = rx_data,
    .len = sizeof(rx_data),
    },
  };

  const struct spi_buf_set rx = {
    .buffers = rx_buf,
    .count = 1,
  };

  int ret = spi_read_dt(&config->bus, &rx);

  *buf = (rx_data[0] << 16) | (rx_data[1] << 8) | rx_data[2];
  uint32_t angle_spi = *buf >> 10;
  uint8_t field_status = (*buf >>6) & 0x3;
  uint8_t push_status = (*buf >> 8) & 0x1;
  uint8_t loss_status = (*buf >>9) & 0x1;
  int new_angle = (int) ((float)angle_spi * 360 / 16384 * 1000);
  LOG_INF("spi angle: %d", new_angle);
  LOG_INF("spi field_status: %d", field_status);
  LOG_INF("spi push_status: %d", push_status);
  LOG_INF("spi loss_status: %d", loss_status);
  return ret;
}

static void mt6701_report_data(const struct device *dev) {
  int ret;
  uint32_t read_data;
  ret = mt6701_read(dev, &read_data);
  if (ret < 0) {
    LOG_ERR("read status: %d", ret);
    return;
  }

  input_report_abs(dev, INPUT_ABS_RZ, read_data, true, K_FOREVER);
  return;
}

static int mt6701_init(const struct device *dev) {
  struct mt6701_data *data = dev->data;
  const struct mt6701_config *config = dev->config;
  LOG_WRN("MT6701 start");
  int ret;
  k_msleep(50);
  mt6701_report_data(dev);
  return 0;
}

#define MT6701_INST(n) \
  static struct mt6701_data mt6701_data_##n; \
  static const struct mt6701_config mt6701_config_##n = { \
    .bus = SPI_DT_SPEC_INST_GET(0, \
                                SPI_OP_MODE_MASTER |  \
                                SPI_WORD_SET(8) | \
                                SPI_LINES_SINGLE | \
                                SPI_TRANSFER_MSB | \
                                SPI_MODE_CPOL, \
                                0), \
  }; \
  DEVICE_DT_INST_DEFINE(n, mt6701_init, NULL, &mt6701_data_##n, \
                        &mt6701_config_##n, POST_KERNEL, \
                        CONFIG_SENSOR_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MT6701_INST)
