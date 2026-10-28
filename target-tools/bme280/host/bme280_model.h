/* Generated BME280 CoreModel register model.
 * Evidence: coremodel.h, examples/i2c/coremodel-i2c.c, examples/i2c-ds3231-rtc/i2c-ds3231.c.
 */
#ifndef BME280_MODEL_H
#define BME280_MODEL_H

#include <stdbool.h>
#include <stdint.h>
#include <coremodel.h>

#define BME280_I2C_ADDRESS 0x76u
#define BME280_REG_ID 0xD0u
#define BME280_CHIP_ID 0x60u
#define BME280_REG_RESET 0xE0u
#define BME280_RESET_CMD 0xB6u

typedef struct BME280Model {
    uint8_t reg[256];
    uint8_t calib[256];
    uint8_t index;
    bool expect_index;
    bool i2c_disabled_until_por;
    uint32_t raw_pressure;
    uint32_t raw_temperature;
    uint16_t raw_humidity;
} BME280Model;

void bme280_init(BME280Model *s);
void bme280_power_on_reset(BME280Model *s);
void bme280_set_raw_pressure(BME280Model *s, uint32_t raw20);
void bme280_set_raw_temperature(BME280Model *s, uint32_t raw20);
void bme280_set_raw_humidity(BME280Model *s, uint16_t raw16);
void bme280_set_calibration_byte(BME280Model *s, uint8_t addr, uint8_t value);
uint8_t bme280_register_read(BME280Model *s, uint8_t addr);
void bme280_register_write(BME280Model *s, uint8_t addr, uint8_t value);
bool bme280_irq_pending(const BME280Model *s);

int bme280_i2c_start(void *priv);
int bme280_i2c_write(void *priv, unsigned len, uint8_t *data);
int bme280_i2c_read(void *priv, unsigned len, uint8_t *data);
void bme280_i2c_stop(void *priv);
const coremodel_i2c_func_t *bme280_coremodel_i2c_func(void);

#endif
