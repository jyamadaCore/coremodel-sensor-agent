/*
 * spi_dac_tool: Linux spidev master for the CoreModel ExampleSpiDacActuator.
 *
 * This is a hand-written test client meant to run on the i.MX93 target
 * (Linux userspace) to exercise the model attached via
 * sensor-models/example-spi-dac-actuator/run-example-spi-dac-actuator.
 * It is not part of that generated model and is not produced by the
 * sensor-model-builder agent.
 *
 * Protocol (sensor-models/example-spi-dac-actuator/README.md):
 *   byte0 = (read ? 0x80 : 0x00) | (reg & 0x7f)
 *   byte1 = data low byte
 *   byte2 = data high byte
 * All three bytes must clock out in a single CS assertion. bits_per_word=24
 * forces the LPSPI controller to hold CS for the whole 3-byte frame, the same
 * workaround documented for spidev_test in coremodel/examples/spi-ds3234-rtc.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

#define REG_CTRL        0x00u
#define REG_DAC_CODE    0x01u
#define REG_STATUS      0x02u
#define REG_DEVICE_ID   0x0fu
#define READ_FLAG       0x80u

#define CTRL_ENABLE_MASK      0x0001u
#define CTRL_RANGE_SEL_MASK   0x0002u
#define DAC_CODE_MASK         0x0fffu

static int spi_fd = -1;

static int spi_xfer(uint8_t reg, int is_read, uint16_t wval, uint16_t *rval)
{
    uint8_t tx[3];
    uint8_t rx[3] = {0, 0, 0};
    struct spi_ioc_transfer xfer[3];
    unsigned i;

    tx[0] = (uint8_t)((is_read ? READ_FLAG : 0u) | (reg & 0x7fu));
    tx[1] = (uint8_t)(wval & 0xffu);
    tx[2] = (uint8_t)((wval >> 8) & 0xffu);

    /* Three chained 1-byte, 8-bit-word transfers in a single ioctl message
     * keep CS asserted across all 3 bytes without relying on a non-standard
     * bits_per_word (some LPSPI/spidev combinations reject e.g. 24). CS is
     * only released after the last transfer in the message completes. */
    memset(xfer, 0, sizeof(xfer));
    for (i = 0; i < 3; i++) {
        xfer[i].tx_buf = (unsigned long)&tx[i];
        xfer[i].rx_buf = (unsigned long)&rx[i];
        xfer[i].len = 1;
        xfer[i].speed_hz = 500000;
    }

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(3), xfer) < 3) {
        fprintf(stderr, "error: spi transfer failed: %s\n", strerror(errno));
        return -1;
    }

    if (rval) {
        *rval = (uint16_t)(rx[1] | ((uint16_t)rx[2] << 8));
    }
    return 0;
}

static int do_read(uint8_t reg, uint16_t *value)
{
    return spi_xfer(reg, 1, 0, value);
}

static int do_write(uint8_t reg, uint16_t value)
{
    return spi_xfer(reg, 0, value, NULL);
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s <spidev-path> <command> [args]\n"
        "  id                     read DEVICE_ID (expect 0x5ac1)\n"
        "  status                 read STATUS (BUSY/OVERRANGE)\n"
        "  ctrl                   read CTRL\n"
        "  ctrl <enable> <range>  write CTRL (enable/range are 0 or 1)\n"
        "  code                   read DAC_CODE\n"
        "  code <value>           write DAC_CODE (0-4095)\n"
        "  read <reg>             generic register read (0x00-0x7f)\n"
        "  write <reg> <value>    generic register write\n"
        "  diag                   probe this spidev node and report what works\n",
        prog);
}

/* Probes a sequence of increasingly specific ioctl calls against a fresh,
 * un-configured spidev fd and reports exactly which one(s) fail and why.
 * Intended to localize an EINVAL without needing another round trip. */
static int run_diag(const char *path)
{
    int fd;
    int v;
    uint8_t tx1 = 0x8f, rx1 = 0;
    uint8_t tx3[3] = {0x8f, 0x00, 0x00};
    uint8_t rx3[3] = {0, 0, 0};
    struct spi_ioc_transfer x1;
    struct spi_ioc_transfer x3;
    struct spi_ioc_transfer x3chain[3];
    unsigned i;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open(%s) failed: %s\n", path, strerror(errno));
        return 1;
    }
    printf("opened %s fd=%d\n", path, fd);

    if (ioctl(fd, SPI_IOC_RD_MODE, &v) < 0) {
        printf("SPI_IOC_RD_MODE: ioctl failed: %s\n", strerror(errno));
    } else {
        printf("default mode = 0x%x\n", v);
    }
    if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &v) < 0) {
        printf("SPI_IOC_RD_BITS_PER_WORD: ioctl failed: %s\n", strerror(errno));
    } else {
        printf("default bits_per_word = %d\n", v);
    }
    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &v) < 0) {
        printf("SPI_IOC_RD_MAX_SPEED_HZ: ioctl failed: %s\n", strerror(errno));
    } else {
        printf("default max_speed_hz = %d\n", v);
    }

    memset(&x1, 0, sizeof(x1));
    x1.tx_buf = (unsigned long)&tx1;
    x1.rx_buf = (unsigned long)&rx1;
    x1.len = 1;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &x1) < 1) {
        printf("probe A (1 byte, all defaults): FAILED: %s\n", strerror(errno));
    } else {
        printf("probe A (1 byte, all defaults): OK, rx=0x%02x\n", rx1);
    }

    memset(&x3, 0, sizeof(x3));
    x3.tx_buf = (unsigned long)tx3;
    x3.rx_buf = (unsigned long)rx3;
    x3.len = 3;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &x3) < 1) {
        printf("probe B (3 bytes, one transfer, all defaults): FAILED: %s\n", strerror(errno));
    } else {
        printf("probe B (3 bytes, one transfer, all defaults): OK, rx=%02x %02x %02x\n",
               rx3[0], rx3[1], rx3[2]);
    }

    memset(x3chain, 0, sizeof(x3chain));
    for (i = 0; i < 3; i++) {
        x3chain[i].tx_buf = (unsigned long)&tx3[i];
        x3chain[i].rx_buf = (unsigned long)&rx3[i];
        x3chain[i].len = 1;
    }
    if (ioctl(fd, SPI_IOC_MESSAGE(3), x3chain) < 3) {
        printf("probe C (3x 1-byte chained, all defaults): FAILED: %s\n", strerror(errno));
    } else {
        printf("probe C (3x 1-byte chained, all defaults): OK, rx=%02x %02x %02x\n",
               rx3[0], rx3[1], rx3[2]);
    }

    v = SPI_MODE_0;
    if (ioctl(fd, SPI_IOC_WR_MODE, &v) < 0) {
        printf("SPI_IOC_WR_MODE(0): FAILED: %s\n", strerror(errno));
    } else {
        printf("SPI_IOC_WR_MODE(0): OK\n");
    }
    v = 8;
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &v) < 0) {
        printf("SPI_IOC_WR_BITS_PER_WORD(8): FAILED: %s\n", strerror(errno));
    } else {
        printf("SPI_IOC_WR_BITS_PER_WORD(8): OK\n");
    }
    v = 500000;
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &v) < 0) {
        printf("SPI_IOC_WR_MAX_SPEED_HZ(500000): FAILED: %s\n", strerror(errno));
    } else {
        printf("SPI_IOC_WR_MAX_SPEED_HZ(500000): OK\n");
    }

    if (ioctl(fd, SPI_IOC_MESSAGE(3), x3chain) < 3) {
        printf("probe D (3x 1-byte chained, after forcing mode/bits/speed): FAILED: %s\n",
               strerror(errno));
    } else {
        printf("probe D (3x 1-byte chained, after forcing mode/bits/speed): OK, rx=%02x %02x %02x\n",
               rx3[0], rx3[1], rx3[2]);
    }

    close(fd);
    return 0;
}

int main(int argc, char **argv)
{
    const char *path;
    const char *cmd;
    uint16_t value = 0;
    int mode = SPI_MODE_0;
    int bits = 8;
    uint32_t speed = 500000;
    int rc = 0;

    if (argc < 3) {
        usage(argv[0]);
        return 2;
    }

    path = argv[1];
    cmd = argv[2];

    if (strcmp(cmd, "diag") == 0) {
        return run_diag(path);
    }

    spi_fd = open(path, O_RDWR);
    if (spi_fd < 0) {
        fprintf(stderr, "error: open(%s) failed: %s\n", path, strerror(errno));
        return 1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        fprintf(stderr, "error: spi configuration failed: %s\n", strerror(errno));
        close(spi_fd);
        return 1;
    }

    if (strcmp(cmd, "id") == 0) {
        rc = do_read(REG_DEVICE_ID, &value);
        if (!rc) printf("DEVICE_ID = 0x%04x\n", value);
    } else if (strcmp(cmd, "status") == 0) {
        rc = do_read(REG_STATUS, &value);
        if (!rc) printf("STATUS = 0x%04x (BUSY=%u OVERRANGE=%u)\n",
                         value, value & 1u, (value >> 1) & 1u);
    } else if (strcmp(cmd, "ctrl") == 0) {
        if (argc == 3) {
            rc = do_read(REG_CTRL, &value);
            if (!rc) printf("CTRL = 0x%04x (ENABLE=%u RANGE_SEL=%u)\n",
                             value, value & 1u, (value >> 1) & 1u);
        } else if (argc == 5) {
            unsigned enable = (unsigned)strtoul(argv[3], NULL, 0);
            unsigned range = (unsigned)strtoul(argv[4], NULL, 0);
            value = (uint16_t)((enable ? CTRL_ENABLE_MASK : 0u) |
                                (range ? CTRL_RANGE_SEL_MASK : 0u));
            rc = do_write(REG_CTRL, value);
            if (!rc) printf("wrote CTRL = 0x%04x\n", value);
        } else {
            usage(argv[0]);
            rc = 2;
        }
    } else if (strcmp(cmd, "code") == 0) {
        if (argc == 3) {
            rc = do_read(REG_DAC_CODE, &value);
            if (!rc) printf("DAC_CODE = %u (0x%03x)\n", value, value);
        } else if (argc == 4) {
            unsigned long code = strtoul(argv[3], NULL, 0);
            rc = do_write(REG_DAC_CODE, (uint16_t)(code & DAC_CODE_MASK));
            if (!rc) printf("wrote DAC_CODE = %lu\n", code & DAC_CODE_MASK);
        } else {
            usage(argv[0]);
            rc = 2;
        }
    } else if (strcmp(cmd, "read") == 0 && argc == 4) {
        unsigned reg = (unsigned)strtoul(argv[3], NULL, 0);
        rc = do_read((uint8_t)reg, &value);
        if (!rc) printf("REG[0x%02x] = 0x%04x\n", reg, value);
    } else if (strcmp(cmd, "write") == 0 && argc == 5) {
        unsigned reg = (unsigned)strtoul(argv[3], NULL, 0);
        unsigned long val = strtoul(argv[4], NULL, 0);
        rc = do_write((uint8_t)reg, (uint16_t)val);
        if (!rc) printf("wrote REG[0x%02x] = 0x%04x\n", reg, (unsigned)val);
    } else {
        usage(argv[0]);
        rc = 2;
    }

    close(spi_fd);
    return rc ? 1 : 0;
}
