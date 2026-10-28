#include "bme280_model.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_reset_and_identity(void)
{
    BME280Model s;
    bme280_init(&s);
    assert(bme280_register_read(&s, 0xD0u) == 0x60u);
    assert(bme280_register_read(&s, 0xF2u) == 0x00u);
    assert(bme280_register_read(&s, 0xF3u) == 0x00u);
    assert(bme280_register_read(&s, 0xF4u) == 0x00u);
    assert(bme280_register_read(&s, 0xF5u) == 0x00u);
    assert(bme280_register_read(&s, 0xF7u) == 0x80u);
    assert(bme280_register_read(&s, 0xFAu) == 0x80u);
    assert(bme280_register_read(&s, 0xFDu) == 0x80u);
}

static void test_access_and_field_masks(void)
{
    BME280Model s;
    bme280_init(&s);
    bme280_register_write(&s, 0xD0u, 0x00u);
    assert(bme280_register_read(&s, 0xD0u) == 0x60u);
    bme280_register_write(&s, 0xF2u, 0xFFu);
    assert(bme280_register_read(&s, 0xF2u) == 0x07u);
    bme280_register_write(&s, 0xF5u, 0xFFu);
    assert(bme280_register_read(&s, 0xF5u) == 0xFDu);
    bme280_register_write(&s, 0xF3u, 0xFFu);
    assert(bme280_register_read(&s, 0xF3u) == 0x00u);
}

static void test_measurement_setters_and_reserved_nibbles(void)
{
    BME280Model s;
    bme280_init(&s);
    bme280_set_raw_pressure(&s, 0xABCDEu);
    bme280_set_raw_temperature(&s, 0x12345u);
    bme280_set_raw_humidity(&s, 0xBEEFu);
    assert(bme280_register_read(&s, 0xF7u) == 0xABu);
    assert(bme280_register_read(&s, 0xF8u) == 0xCDu);
    assert(bme280_register_read(&s, 0xF9u) == 0xE0u);
    assert((bme280_register_read(&s, 0xF9u) & 0x0Fu) == 0x00u);
    assert(bme280_register_read(&s, 0xFAu) == 0x12u);
    assert(bme280_register_read(&s, 0xFBu) == 0x34u);
    assert(bme280_register_read(&s, 0xFCu) == 0x50u);
    assert((bme280_register_read(&s, 0xFCu) & 0x0Fu) == 0x00u);
    assert(bme280_register_read(&s, 0xFDu) == 0xBEu);
    assert(bme280_register_read(&s, 0xFEu) == 0xEFu);
}

static void test_side_effects_and_calibration(void)
{
    BME280Model s;
    bme280_init(&s);
    bme280_set_calibration_byte(&s, 0x88u, 0x34u);
    bme280_set_calibration_byte(&s, 0xE7u, 0x9Au);
    bme280_register_write(&s, 0xF2u, 0x05u);
    bme280_register_write(&s, 0xF4u, 0x25u);
    assert((bme280_register_read(&s, 0xF4u) & 0x03u) == 0x00u);
    bme280_register_write(&s, 0xE0u, 0x00u);
    assert(bme280_register_read(&s, 0xF2u) == 0x05u);
    bme280_register_write(&s, 0xE0u, 0xB6u);
    assert(bme280_register_read(&s, 0xF2u) == 0x00u);
    assert(bme280_register_read(&s, 0x88u) == 0x34u);
    assert(bme280_register_read(&s, 0xE7u) == 0x9Au);
    assert(bme280_register_read(&s, 0xE0u) == 0x00u);
}

static void test_i2c_framing(void)
{
    BME280Model s;
    uint8_t w1[] = {0xF2u, 0x03u, 0xF5u, 0xA5u};
    uint8_t set_index[] = {0xF7u};
    uint8_t r[8] = {0};
    bme280_init(&s);
    assert(bme280_i2c_start(&s) == 1);
    assert(bme280_i2c_write(&s, sizeof(w1), w1) == (int)sizeof(w1));
    bme280_i2c_stop(&s);
    assert(bme280_register_read(&s, 0xF2u) == 0x03u);
    assert(bme280_register_read(&s, 0xF5u) == 0xA5u);
    bme280_set_raw_pressure(&s, 0xABCDEu);
    bme280_set_raw_temperature(&s, 0x12345u);
    assert(bme280_i2c_start(&s) == 1);
    assert(bme280_i2c_write(&s, sizeof(set_index), set_index) == (int)sizeof(set_index));
    assert(bme280_i2c_read(&s, sizeof(r), r) == (int)sizeof(r));
    assert(r[0] == 0xABu && r[1] == 0xCDu && r[2] == 0xE0u);
    assert(r[3] == 0x12u && r[4] == 0x34u && r[5] == 0x50u);
}

static void test_invalid_access_and_interrupts(void)
{
    BME280Model s;
    bme280_init(&s);
    bme280_register_write(&s, 0x00u, 0xFFu);
    assert(bme280_register_read(&s, 0x00u) == 0x00u);
    assert(!bme280_irq_pending(&s));
    s.i2c_disabled_until_por = true;
    assert(bme280_i2c_start(&s) == -1);
}

int main(void)
{
    test_reset_and_identity();
    test_access_and_field_masks();
    test_measurement_setters_and_reserved_nibbles();
    test_side_effects_and_calibration();
    test_i2c_framing();
    test_invalid_access_and_interrupts();
    puts("BME280 tests passed");
    return 0;
}
