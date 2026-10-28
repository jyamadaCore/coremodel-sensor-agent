/*
 * bme280_coremodel_inject: attaches the generated BME280Model to CoreModel
 * exactly like sensor-models/bme280/src/bme280_coremodel_main.c does, but
 * first loads a synthetic (not from any real chip) calibration table and
 * raw ADC reading via bme280_set_calibration_byte() / bme280_set_raw_*().
 *
 * The bare bme280-coremodel binary leaves calibration at its documented
 * placeholder of all-zero bytes (sensor-models/bme280/README.md), which
 * forces every compensated reading to 0.00 regardless of raw ADC data --
 * expected, but useless for exercising target-tools/bme280/target's real
 * compensation math against a virtual bus. This harness exists only to
 * give that target tool something non-zero to read.
 *
 * bme280_model.c's complete_forced_measurement() only resets pressure/
 * temperature/humidity to the power-on placeholder when the oversampling
 * for that measurement is disabled; target/bme280_i2c_tool.c always
 * requests oversampling x1 on all three, so the values injected here
 * survive its forced-mode trigger untouched.
 *
 * This is hand-written debug/demo tooling, not output of the
 * sensor-model-builder agent -- kept out of sensor-models/bme280/ since
 * that directory is reserved for the agent's generated files.
 * bme280_model.{c,h} and coremodel.{c,h} in this folder are vendored
 * copies of sensor-models/bme280/src/ and
 * target-tools/example-spi-dac-actuator/host/coremodel.{c,h}; re-copy
 * bme280_model.{c,h} here if the agent regenerates that model.
 */
#include "bme280_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coremodel.h"

static void set_calib16(BME280Model *s, uint8_t reg_lo, int32_t val)
{
    uint16_t u = (uint16_t)val;
    bme280_set_calibration_byte(s, reg_lo, (uint8_t)(u & 0xFFu));
    bme280_set_calibration_byte(s, (uint8_t)(reg_lo + 1u), (uint8_t)((u >> 8) & 0xFFu));
}

/* Loads a self-consistent synthetic calibration table and raw ADC reading.
 * Computed (not from a real chip) to compensate to approximately
 * T=25.1 C, P=1006.5 hPa, H=21.3 %RH. */
static void load_demo_data(BME280Model *s)
{
    const int32_t dig_T1 = 27504, dig_T2 = 26435, dig_T3 = -1000;
    const int32_t dig_P1 = 36477, dig_P2 = -10685, dig_P3 = 3024, dig_P4 = 2855,
                  dig_P5 = 140, dig_P6 = -7, dig_P7 = 15500, dig_P8 = -14600, dig_P9 = 6000;
    const int32_t dig_H1 = 75, dig_H2 = 355, dig_H3 = 0, dig_H4 = 309, dig_H5 = 0, dig_H6 = 30;
    uint8_t e4, e5, e6;

    set_calib16(s, 0x88u, dig_T1);
    set_calib16(s, 0x8Au, dig_T2);
    set_calib16(s, 0x8Cu, dig_T3);
    set_calib16(s, 0x8Eu, dig_P1);
    set_calib16(s, 0x90u, dig_P2);
    set_calib16(s, 0x92u, dig_P3);
    set_calib16(s, 0x94u, dig_P4);
    set_calib16(s, 0x96u, dig_P5);
    set_calib16(s, 0x98u, dig_P6);
    set_calib16(s, 0x9Au, dig_P7);
    set_calib16(s, 0x9Cu, dig_P8);
    set_calib16(s, 0x9Eu, dig_P9);
    bme280_set_calibration_byte(s, 0xA1u, (uint8_t)dig_H1);

    set_calib16(s, 0xE1u, dig_H2);
    bme280_set_calibration_byte(s, 0xE3u, (uint8_t)dig_H3);
    /* dig_H4/dig_H5 are packed across E4/E5/E6; see the matching decode in
     * target-tools/bme280/target/bme280_i2c_tool.c's read_calibration(). */
    e4 = (uint8_t)(dig_H4 >> 4);
    e6 = (uint8_t)(dig_H5 >> 4);
    e5 = (uint8_t)(((dig_H5 & 0x0F) << 4) | (dig_H4 & 0x0F));
    bme280_set_calibration_byte(s, 0xE4u, e4);
    bme280_set_calibration_byte(s, 0xE5u, e5);
    bme280_set_calibration_byte(s, 0xE6u, e6);
    bme280_set_calibration_byte(s, 0xE7u, (uint8_t)dig_H6);

    bme280_set_raw_temperature(s, 519888u);
    bme280_set_raw_pressure(s, 415148u);
    bme280_set_raw_humidity(s, 23627u);
}

int main(int argc, char **argv)
{
    void *cm = NULL;
    void *handle = NULL;
    int res;
    BME280Model *state;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <address[:port]> <i2c-bus>\n", argv[0]);
        return 1;
    }

    state = calloc(1, sizeof(*state));
    if (state == NULL) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }
    bme280_init(state);
    load_demo_data(state);

    printf("BME280 CoreModel injector: synthetic calibration + raw ADC data loaded.\n");
    printf("Expect roughly T=25.1 C  P=1006.5 hPa  H=21.3 %%RH from bme280_i2c_tool.\n");
    fflush(stdout);

    res = coremodel_connect(&cm, argv[1]);
    if (res != 0) {
        fprintf(stderr, "error: failed to connect: %s\n", strerror(-res));
        free(state);
        return 1;
    }

    handle = coremodel_attach_i2c(cm, argv[2], BME280_I2C_ADDRESS,
                                   bme280_coremodel_i2c_func(), state, 0);
    if (handle == NULL) {
        fprintf(stderr, "error: failed to attach BME280 at 0x%02x\n", BME280_I2C_ADDRESS);
        coremodel_disconnect(cm);
        free(state);
        return 1;
    }

    res = coremodel_mainloop(cm, -1);
    coremodel_detach(handle);
    coremodel_disconnect(cm);
    free(state);
    return res ? 1 : 0;
}
