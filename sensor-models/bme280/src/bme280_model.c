/* Generated BME280 CoreModel register model.
 * CoreModel API evidence: coremodel.h, examples/i2c/coremodel-i2c.c, examples/i2c-ds3231-rtc/i2c-ds3231.c.
 */
#include "bme280_model.h"

#include <stddef.h>
#include <string.h>

#define REG_CALIB0_START 0x88u
#define REG_CALIB0_END   0xA1u
#define REG_CALIB1_START 0xE1u
#define REG_CALIB1_END   0xE7u
#define REG_CTRL_HUM     0xF2u
#define REG_STATUS       0xF3u
#define REG_CTRL_MEAS    0xF4u
#define REG_CONFIG       0xF5u
#define REG_PRESS_MSB    0xF7u
#define REG_PRESS_LSB    0xF8u
#define REG_PRESS_XLSB   0xF9u
#define REG_TEMP_MSB     0xFAu
#define REG_TEMP_LSB     0xFBu
#define REG_TEMP_XLSB    0xFCu
#define REG_HUM_MSB      0xFDu
#define REG_HUM_LSB      0xFEu

static bool is_calib(uint8_t addr)
{
    return ((addr >= REG_CALIB0_START) && (addr <= REG_CALIB0_END)) ||
           ((addr >= REG_CALIB1_START) && (addr <= REG_CALIB1_END));
}

static bool is_ro(uint8_t addr)
{
    return is_calib(addr) || (addr == BME280_REG_ID) || (addr == REG_STATUS) ||
           ((addr >= REG_PRESS_MSB) && (addr <= REG_HUM_LSB));
}

static bool is_rw(uint8_t addr)
{
    return (addr == REG_CTRL_HUM) || (addr == REG_CTRL_MEAS) || (addr == REG_CONFIG);
}

static void encode_pressure(BME280Model *s, uint32_t raw20)
{
    raw20 &= 0xFFFFFu;
    s->raw_pressure = raw20;
    s->reg[REG_PRESS_MSB] = (uint8_t)(raw20 >> 12);
    s->reg[REG_PRESS_LSB] = (uint8_t)(raw20 >> 4);
    s->reg[REG_PRESS_XLSB] = (uint8_t)((raw20 & 0x0Fu) << 4);
}

static void encode_temperature(BME280Model *s, uint32_t raw20)
{
    raw20 &= 0xFFFFFu;
    s->raw_temperature = raw20;
    s->reg[REG_TEMP_MSB] = (uint8_t)(raw20 >> 12);
    s->reg[REG_TEMP_LSB] = (uint8_t)(raw20 >> 4);
    s->reg[REG_TEMP_XLSB] = (uint8_t)((raw20 & 0x0Fu) << 4);
}

static void encode_humidity(BME280Model *s, uint16_t raw16)
{
    s->raw_humidity = raw16;
    s->reg[REG_HUM_MSB] = (uint8_t)(raw16 >> 8);
    s->reg[REG_HUM_LSB] = (uint8_t)raw16;
}

static void restore_calibration(BME280Model *s)
{
    for(unsigned a = REG_CALIB0_START; a <= REG_CALIB0_END; a++) {
        s->reg[a] = s->calib[a];
    }
    for(unsigned a = REG_CALIB1_START; a <= REG_CALIB1_END; a++) {
        s->reg[a] = s->calib[a];
    }
}

void bme280_power_on_reset(BME280Model *s)
{
    memset(s->reg, 0, sizeof(s->reg));
    restore_calibration(s);
    s->reg[BME280_REG_ID] = BME280_CHIP_ID;
    s->reg[BME280_REG_RESET] = 0x00u;
    s->reg[REG_STATUS] = 0x00u;
    s->reg[REG_CTRL_HUM] = 0x00u;
    s->reg[REG_CTRL_MEAS] = 0x00u;
    s->reg[REG_CONFIG] = 0x00u;
    encode_pressure(s, 0x80000u);
    encode_temperature(s, 0x80000u);
    encode_humidity(s, 0x8000u);
    s->index = 0u;
    s->expect_index = true;
    s->i2c_disabled_until_por = false;
}

void bme280_init(BME280Model *s)
{
    memset(s, 0, sizeof(*s));
    bme280_power_on_reset(s);
}

void bme280_set_raw_pressure(BME280Model *s, uint32_t raw20)
{
    encode_pressure(s, raw20);
}

void bme280_set_raw_temperature(BME280Model *s, uint32_t raw20)
{
    encode_temperature(s, raw20);
}

void bme280_set_raw_humidity(BME280Model *s, uint16_t raw16)
{
    encode_humidity(s, raw16);
}

void bme280_set_calibration_byte(BME280Model *s, uint8_t addr, uint8_t value)
{
    if(is_calib(addr)) {
        s->calib[addr] = value;
        s->reg[addr] = value;
    }
}

static void complete_forced_measurement(BME280Model *s)
{
    const uint8_t ctrl_hum = s->reg[REG_CTRL_HUM] & 0x07u;
    const uint8_t ctrl_meas = s->reg[REG_CTRL_MEAS];
    const uint8_t osrs_p = (uint8_t)((ctrl_meas >> 2) & 0x07u);
    const uint8_t osrs_t = (uint8_t)((ctrl_meas >> 5) & 0x07u);

    if(osrs_p == 0u) {
        encode_pressure(s, 0x80000u);
    }
    if(osrs_t == 0u) {
        encode_temperature(s, 0x80000u);
    }
    if(ctrl_hum == 0u) {
        encode_humidity(s, 0x8000u);
    }
    s->reg[REG_STATUS] &= (uint8_t)~0x09u;
    s->reg[REG_CTRL_MEAS] &= (uint8_t)~0x03u;
}

uint8_t bme280_register_read(BME280Model *s, uint8_t addr)
{
    if(addr == BME280_REG_RESET) {
        return 0x00u;
    }
    if(is_ro(addr) || is_rw(addr)) {
        return s->reg[addr];
    }
    return 0x00u;
}

void bme280_register_write(BME280Model *s, uint8_t addr, uint8_t value)
{
    if(addr == BME280_REG_RESET) {
        if(value == BME280_RESET_CMD) {
            bme280_power_on_reset(s);
        }
        return;
    }
    if(is_ro(addr)) {
        return;
    }
    switch(addr) {
    case REG_CTRL_HUM:
        s->reg[addr] = (uint8_t)((s->reg[addr] & 0xF8u) | (value & 0x07u));
        break;
    case REG_CTRL_MEAS:
        s->reg[addr] = value;
        if((value & 0x03u) == 0x01u || (value & 0x03u) == 0x02u) {
            complete_forced_measurement(s);
        }
        break;
    case REG_CONFIG:
        s->reg[addr] = (uint8_t)((s->reg[addr] & 0x02u) | (value & 0xFDu));
        break;
    default:
        break;
    }
}

bool bme280_irq_pending(const BME280Model *s)
{
    (void)s;
    return false;
}

int bme280_i2c_start(void *priv)
{
    BME280Model *s = (BME280Model *)priv;
    if(s->i2c_disabled_until_por) {
        return -1;
    }
    s->expect_index = true;
    return 1;
}

int bme280_i2c_write(void *priv, unsigned len, uint8_t *data)
{
    BME280Model *s = (BME280Model *)priv;
    unsigned i = 0;

    if(s->i2c_disabled_until_por) {
        return -1;
    }
    if(len == 0u) {
        return -1;
    }
    while(i < len) {
        if(s->expect_index) {
            s->index = data[i++];
            s->expect_index = false;
            if(i >= len) {
                break;
            }
        }
        bme280_register_write(s, s->index, data[i++]);
        if(i < len) {
            s->expect_index = true;
        }
    }
    return (int)len;
}

int bme280_i2c_read(void *priv, unsigned len, uint8_t *data)
{
    BME280Model *s = (BME280Model *)priv;
    unsigned i;

    if(s->i2c_disabled_until_por) {
        return 0;
    }
    for(i = 0; i < len; i++) {
        data[i] = bme280_register_read(s, s->index);
        s->index++;
    }
    return (int)len;
}

void bme280_i2c_stop(void *priv)
{
    BME280Model *s = (BME280Model *)priv;
    s->expect_index = true;
}

static const coremodel_i2c_func_t bme280_i2c_func = {
    .start = bme280_i2c_start,
    .write = bme280_i2c_write,
    .read = bme280_i2c_read,
    .stop = bme280_i2c_stop,
};

const coremodel_i2c_func_t *bme280_coremodel_i2c_func(void)
{
    return &bme280_i2c_func;
}
