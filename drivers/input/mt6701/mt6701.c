#include <sys/errno.h>
#define DT_DRV_COMPAT magntek_mt6701

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include "zephyr/devicetree.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include "zephyr/drivers/sensor.h"
#include <zephyr/drivers/spi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mt6701, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t tableCRC6[64] = {
 0x00, 0x03, 0x06, 0x05, 0x0C, 0x0F, 0x0A, 0x09,
 0x18, 0x1B, 0x1E, 0x1D, 0x14, 0x17, 0x12, 0x11,
 0x30, 0x33, 0x36, 0x35, 0x3C, 0x3F, 0x3A, 0x39,
 0x28, 0x2B, 0x2E, 0x2D, 0x24, 0x27, 0x22, 0x21,
 0x23, 0x20, 0x25, 0x26, 0x2F, 0x2C, 0x29, 0x2A,
 0x3B, 0x38, 0x3D, 0x3E, 0x37, 0x34, 0x31, 0x32,
 0x13, 0x10, 0x15, 0x16, 0x1F, 0x1C, 0x19, 0x1A,
 0x0B, 0x08, 0x0D, 0x0E, 0x07, 0x04, 0x01, 0x02
};

struct mt6701_data {
  struct sensor_value rotation;
};

struct mt6701_config {
  const struct spi_dt_spec bus;
};

static uint8_t CRC6_43_18bit (uint32_t input_data)
{
 uint8_t b_Index = 0;
 uint8_t b_CRC = 0;

 b_Index = (uint8_t )(((uint32_t)input_data >> 12u) & 0x0000003Fu);

 b_CRC = (uint8_t )(((uint32_t)input_data >> 6u) & 0x0000003Fu);
 b_Index = b_CRC ^ tableCRC6[b_Index];

 b_CRC = (uint8_t )((uint32_t)input_data & 0x0000003Fu);
 b_Index = b_CRC ^ tableCRC6[b_Index];

 b_CRC = tableCRC6[b_Index];

 return b_CRC;
}

static int mt6701_read(const struct device *dev, uint32_t *data) {
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

  uint32_t data_received = (rx_data[0] << 16) | (rx_data[1] << 8) | rx_data[2];
  uint8_t received_crc = data_received & 0x3f;
  uint8_t calculated_crc = CRC6_43_18bit(data_received >> 6);
  if (received_crc == calculated_crc) {
    *data = data_received;
  } else {
    LOG_ERR("CRC does not match");
  }
  return ret;
}


static int mt6701_sample_fetch(const struct device *dev,
                               enum sensor_channel chan) {
  struct mt6701_data *data = dev->data;
  uint32_t data_spi = 0;

  int ret;
  ret = mt6701_read(dev, &data_spi);
  if (ret < 0) {
    LOG_ERR("sample fetched failed: %d", ret);
    return ret;
  }

  uint32_t angle_spi = data_spi >> 10;
  float calculated_angle = (float) angle_spi * 360 / 16384;
  
  int32_t int_part = (int32_t) calculated_angle;
  int32_t frac_part = (int32_t) ((calculated_angle - int_part) * 1000000);
  data->rotation.val1 = int_part;
  data->rotation.val2 = frac_part;

  // uint8_t field_status = (data_spi >>6) & 0x3;
  // uint8_t push_status = (data_spi >> 8) & 0x1;
  // uint8_t loss_status = (data_spi >>9) & 0x1;

  // LOG_INF("angle rounded to nearest degree: %d", (int) calculated_angle);
  // LOG_INF("field_status: %d", field_status);
  // LOG_INF("push_status: %d", push_status);
  // LOG_INF("loss_status: %d", loss_status);

  return 0;
}

static int mt6701_channel_get(const struct device *dev,
                              enum sensor_channel chan,
                              struct sensor_value *val) {
  struct mt6701_data *data = dev->data;
  if (chan != SENSOR_CHAN_ROTATION) {
    return -ENOTSUP;
  }

  *val = data->rotation;

  return 0;
}

static const struct sensor_driver_api mt6701_api = {
  .sample_fetch = &mt6701_sample_fetch,
  .channel_get = &mt6701_channel_get,
};

static int mt6701_init(const struct device *dev) {
  // struct mt6701_data *data = dev->data;
  // const struct mt6701_config *config = dev->config;
  LOG_DBG("MT6701 start");
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
                                SPI_MODE_CPHA, \
                                0), \
  }; \
  DEVICE_DT_INST_DEFINE(n, mt6701_init, NULL, &mt6701_data_##n, \
                        &mt6701_config_##n, POST_KERNEL, \
                        CONFIG_SENSOR_INIT_PRIORITY, &mt6701_api);

DT_INST_FOREACH_STATUS_OKAY(MT6701_INST)
