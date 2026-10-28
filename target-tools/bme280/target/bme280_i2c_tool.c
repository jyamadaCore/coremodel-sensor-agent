/*
 * bme280_i2c_tool: Linux i2c-dev master for a real Bosch BME280, meant to
 * run on the i.MX93 target (Linux userspace) over the LPI2C1 controller's
 * /dev/i2c-N node. Pairs with the CoreModel model at
 * sensor-models/bme280/ (same 7-bit address 0x76) when testing against a
 * virtual bus instead of real silicon.
 *
 * Register-level transactions use the I2C_RDWR ioctl so a register-address
 * write and the following data read happen as one combined transaction
 * with a repeated start, which is what the BME280 datasheet requires for
 * multi-byte reads (status/data/calibration) to stay consistent.
 *
 * Compensation formulas (temperature/pressure/humidity) are the
 * double-precision reference algorithms from the Bosch BME280 datasheet.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>

#define REG_CALIB1      0x88u /* dig_T1..dig_P9, 24 bytes */
#define REG_CALIB_H1    0xA1u /* dig_H1, 1 byte */
#define REG_CALIB2      0xE1u /* dig_H2..dig_H6, 7 bytes */
#define REG_CHIP_ID     0xD0u
#define REG_CTRL_HUM    0xF2u
#define REG_STATUS      0xF3u
#define REG_CTRL_MEAS   0xF4u
#define REG_DATA        0xF7u /* press_msb..hum_lsb, 8 bytes */

#define CTRL_HUM_OSRS_H1   0x01u /* humidity oversampling x1 */
#define CTRL_MEAS_FORCED   0x25u /* temp x1, press x1, mode=forced */
#define STATUS_MEASURING   0x08u

struct bme280_calib {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4, dig_H5;
    int8_t   dig_H6;
};

static int i2c_read_regs(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data xfer;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = (uint16_t)len;
    msgs[1].buf = buf;

    xfer.msgs = msgs;
    xfer.nmsgs = 2;

    if (ioctl(fd, I2C_RDWR, &xfer) < 0) {
        fprintf(stderr, "error: i2c read reg 0x%02x failed: %s\n", reg, strerror(errno));
        return -1;
    }
    return 0;
}

static int i2c_write_reg(int fd, uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t tx[2];
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data xfer;

    tx[0] = reg;
    tx[1] = val;

    msg.addr = addr;
    msg.flags = 0;
    msg.len = sizeof(tx);
    msg.buf = tx;

    xfer.msgs = &msg;
    xfer.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &xfer) < 0) {
        fprintf(stderr, "error: i2c write reg 0x%02x failed: %s\n", reg, strerror(errno));
        return -1;
    }
    return 0;
}

static int read_calibration(int fd, uint8_t addr, struct bme280_calib *c)
{
    uint8_t b1[24];
    uint8_t h1;
    uint8_t b2[7];

    if (i2c_read_regs(fd, addr, REG_CALIB1, b1, sizeof(b1)) < 0) {
        return -1;
    }
    if (i2c_read_regs(fd, addr, REG_CALIB_H1, &h1, 1) < 0) {
        return -1;
    }
    if (i2c_read_regs(fd, addr, REG_CALIB2, b2, sizeof(b2)) < 0) {
        return -1;
    }

    c->dig_T1 = (uint16_t)(b1[0] | (b1[1] << 8));
    c->dig_T2 = (int16_t)(b1[2] | (b1[3] << 8));
    c->dig_T3 = (int16_t)(b1[4] | (b1[5] << 8));
    c->dig_P1 = (uint16_t)(b1[6] | (b1[7] << 8));
    c->dig_P2 = (int16_t)(b1[8] | (b1[9] << 8));
    c->dig_P3 = (int16_t)(b1[10] | (b1[11] << 8));
    c->dig_P4 = (int16_t)(b1[12] | (b1[13] << 8));
    c->dig_P5 = (int16_t)(b1[14] | (b1[15] << 8));
    c->dig_P6 = (int16_t)(b1[16] | (b1[17] << 8));
    c->dig_P7 = (int16_t)(b1[18] | (b1[19] << 8));
    c->dig_P8 = (int16_t)(b1[20] | (b1[21] << 8));
    c->dig_P9 = (int16_t)(b1[22] | (b1[23] << 8));

    c->dig_H1 = h1;
    /* dig_H4/dig_H5 are packed across E4/E5/E6 (b2[3..5]) per the BME280 datasheet. */
    c->dig_H2 = (int16_t)(b2[0] | (b2[1] << 8));
    c->dig_H3 = b2[2];
    c->dig_H4 = (int16_t)(((int8_t)b2[3]) * 16 + (b2[4] & 0x0Fu));
    c->dig_H5 = (int16_t)(((int8_t)b2[5]) * 16 + (b2[4] >> 4));
    c->dig_H6 = (int8_t)b2[6];

    return 0;
}

static double compensate_temperature(const struct bme280_calib *c, int32_t adc_T, int32_t *t_fine_out)
{
    double var1, var2, T;

    var1 = (((double)adc_T) / 16384.0 - ((double)c->dig_T1) / 1024.0);
    var2 = var1 * ((double)c->dig_T2);
    var1 = (((double)adc_T) / 131072.0 - ((double)c->dig_T1) / 8192.0);
    var1 = var1 * var1 * ((double)c->dig_T3);
    *t_fine_out = (int32_t)(var2 + var1);
    T = (var2 + var1) / 5120.0;
    return T;
}

static double compensate_pressure(const struct bme280_calib *c, int32_t adc_P, int32_t t_fine)
{
    double var1, var2, p;

    var1 = ((double)t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)c->dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)c->dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)c->dig_P4) * 65536.0);
    var1 = (((double)c->dig_P3) * var1 * var1 / 524288.0 + ((double)c->dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)c->dig_P1);
    if (var1 == 0.0) {
        return 0.0;
    }
    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)c->dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)c->dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)c->dig_P7)) / 16.0;
    return p; /* Pa */
}

static double compensate_humidity(const struct bme280_calib *c, int32_t adc_H, int32_t t_fine)
{
    double var_h;

    var_h = ((double)t_fine) - 76800.0;
    var_h = (((double)adc_H) - (((double)c->dig_H4) * 64.0 + ((double)c->dig_H5) / 16384.0 * var_h)) *
            (((double)c->dig_H2) / 65536.0 * (1.0 + ((double)c->dig_H6) / 67108864.0 * var_h *
            (1.0 + ((double)c->dig_H3) / 67108864.0 * var_h)));
    var_h = var_h * (1.0 - ((double)c->dig_H1) * var_h / 524288.0);
    if (var_h > 100.0) {
        var_h = 100.0;
    } else if (var_h < 0.0) {
        var_h = 0.0;
    }
    return var_h; /* %RH */
}

/* Triggers one forced-mode conversion, waits for it to complete, reads the
 * raw ADC bytes, and prints the compensated T/P/H. Forced mode (rather than
 * normal mode's free-running sampling) keeps "one read" and "one fresh
 * conversion" the same thing, which is what both the "once" and "stream"
 * commands below want. */
static int do_once(int fd, uint8_t addr, const struct bme280_calib *c)
{
    uint8_t status = 0;
    uint8_t data[8];
    int32_t adc_P, adc_T, adc_H, t_fine;
    double T, P, H;
    int tries;

    if (i2c_write_reg(fd, addr, REG_CTRL_HUM, CTRL_HUM_OSRS_H1) < 0) {
        return -1;
    }
    if (i2c_write_reg(fd, addr, REG_CTRL_MEAS, CTRL_MEAS_FORCED) < 0) {
        return -1;
    }

    for (tries = 0; tries < 100; tries++) {
        if (i2c_read_regs(fd, addr, REG_STATUS, &status, 1) < 0) {
            return -1;
        }
        if (!(status & STATUS_MEASURING)) {
            break;
        }
        usleep(10000);
    }
    if (status & STATUS_MEASURING) {
        fprintf(stderr, "error: conversion did not complete in time\n");
        return -1;
    }

    if (i2c_read_regs(fd, addr, REG_DATA, data, sizeof(data)) < 0) {
        return -1;
    }

    adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
    adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);
    adc_H = ((int32_t)data[6] << 8) | data[7];

    T = compensate_temperature(c, adc_T, &t_fine);
    P = compensate_pressure(c, adc_P, t_fine);
    H = compensate_humidity(c, adc_H, t_fine);

    printf("T=%.2f C  P=%.2f hPa  H=%.2f %%RH\n", T, P / 100.0, H);
    return 0;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s <i2c-dev-path> <i2c-addr> <command> [args]\n"
        "  id                    read CHIP_ID (0xD0), expect 0x60\n"
        "  read <reg>            raw register read (hex or decimal)\n"
        "  write <reg> <value>   raw register write\n"
        "  dump                  print chip ID, raw calibration, status, raw data (hex)\n"
        "  once                  trigger one forced-mode conversion, print T/P/H\n"
        "  stream [interval_s]   repeat 'once' every interval_s seconds (default 1)\n"
        "                        until Ctrl+C\n"
        "\n"
        "i2c-addr is the BME280's 7-bit slave address: 0x76 with SDO tied low\n"
        "(matches sensor-models/bme280/'s CoreModel address), 0x77 if SDO is\n"
        "tied high.\n",
        prog);
}

int main(int argc, char **argv)
{
    const char *path;
    uint8_t addr;
    const char *cmd;
    int fd;
    struct bme280_calib calib;
    int rc = 0;

    if (argc < 4) {
        usage(argv[0]);
        return 2;
    }

    path = argv[1];
    addr = (uint8_t)strtoul(argv[2], NULL, 0);
    cmd = argv[3];

    fd = open(path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "error: open(%s) failed: %s\n", path, strerror(errno));
        return 1;
    }

    if (strcmp(cmd, "id") == 0) {
        uint8_t id = 0;
        rc = i2c_read_regs(fd, addr, REG_CHIP_ID, &id, 1);
        if (!rc) {
            printf("CHIP_ID = 0x%02x\n", id);
        }
    } else if (strcmp(cmd, "read") == 0 && argc == 5) {
        unsigned reg = (unsigned)strtoul(argv[4], NULL, 0);
        uint8_t val = 0;
        rc = i2c_read_regs(fd, addr, (uint8_t)reg, &val, 1);
        if (!rc) {
            printf("REG[0x%02x] = 0x%02x\n", reg, val);
        }
    } else if (strcmp(cmd, "write") == 0 && argc == 6) {
        unsigned reg = (unsigned)strtoul(argv[4], NULL, 0);
        unsigned long val = strtoul(argv[5], NULL, 0);
        rc = i2c_write_reg(fd, addr, (uint8_t)reg, (uint8_t)val);
        if (!rc) {
            printf("wrote REG[0x%02x] = 0x%02x\n", reg, (unsigned)val);
        }
    } else if (strcmp(cmd, "dump") == 0 && argc == 4) {
        uint8_t id = 0;
        uint8_t b1[24];
        uint8_t h1 = 0;
        uint8_t b2[7];
        uint8_t status = 0;
        uint8_t data[8];
        size_t i;

        rc = i2c_read_regs(fd, addr, REG_CHIP_ID, &id, 1);
        if (!rc) {
            rc = i2c_read_regs(fd, addr, REG_CALIB1, b1, sizeof(b1));
        }
        if (!rc) {
            rc = i2c_read_regs(fd, addr, REG_CALIB_H1, &h1, 1);
        }
        if (!rc) {
            rc = i2c_read_regs(fd, addr, REG_CALIB2, b2, sizeof(b2));
        }
        if (!rc) {
            rc = i2c_read_regs(fd, addr, REG_STATUS, &status, 1);
        }
        if (!rc) {
            rc = i2c_read_regs(fd, addr, REG_DATA, data, sizeof(data));
        }
        if (!rc) {
            printf("CHIP_ID            (0xd0)        = 0x%02x\n", id);
            printf("CALIB1  (0x88..0x9f, 24B, T1-P9) =");
            for (i = 0; i < sizeof(b1); i++) {
                printf(" %02x", b1[i]);
            }
            printf("\n");
            printf("CALIB_H1           (0xa1)        = 0x%02x\n", h1);
            printf("CALIB2  (0xe1..0xe7, 7B, H2-H6)  =");
            for (i = 0; i < sizeof(b2); i++) {
                printf(" %02x", b2[i]);
            }
            printf("\n");
            printf("STATUS             (0xf3)        = 0x%02x\n", status);
            printf("DATA    (0xf7..0xfe, 8B, P/T/H)  =");
            for (i = 0; i < sizeof(data); i++) {
                printf(" %02x", data[i]);
            }
            printf("\n");
        }
    } else if (strcmp(cmd, "once") == 0 && argc == 4) {
        rc = read_calibration(fd, addr, &calib);
        if (!rc) {
            rc = do_once(fd, addr, &calib);
        }
    } else if (strcmp(cmd, "stream") == 0 && argc <= 5) {
        unsigned interval = (argc == 5) ? (unsigned)strtoul(argv[4], NULL, 0) : 1;
        if (interval == 0) {
            interval = 1;
        }
        rc = read_calibration(fd, addr, &calib);
        while (!rc) {
            rc = do_once(fd, addr, &calib);
            if (rc) {
                break;
            }
            sleep(interval);
        }
    } else {
        usage(argv[0]);
        rc = 2;
    }

    close(fd);
    return rc ? 1 : 0;
}
