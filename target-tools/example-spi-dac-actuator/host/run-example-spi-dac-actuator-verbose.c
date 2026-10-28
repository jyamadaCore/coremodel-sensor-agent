/*
 * Verbose CoreModel SPI attachment for ExampleSpiDacActuator.
 *
 * Identical behavior to
 * sensor-models/example-spi-dac-actuator/src/example_spi_dac_actuator_main.c,
 * but traces every CS change and every byte transferred to stdout so you can
 * watch live traffic on the host while driving the model from a target with
 * spi_dac_tool. Kept out of sensor-models/example-spi-dac-actuator/ since
 * that directory is reserved for the sensor-model-builder agent's generated
 * files; this is hand-written debug/demo tooling, not a replacement for it.
 */
#include "example_spi_dac_actuator.h"

#include "coremodel.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void verbose_cs(void *priv, unsigned csel)
{
    printf("CS%u asserted\n", csel);
    fflush(stdout);
    example_spi_dac_actuator_spi_cs(priv, csel);
}

static int verbose_xfr(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata)
{
    int accepted;
    unsigned i;

    printf("TX [%u]", len);
    for (i = 0; i < len; i++) {
        printf(" %02x", wrdata[i]);
    }
    printf("\n");

    accepted = example_spi_dac_actuator_spi_xfr(priv, len, wrdata, rddata);

    printf("RX [%u]", len);
    for (i = 0; i < len; i++) {
        printf(" %02x", rddata[i]);
    }
    printf("\n");
    fflush(stdout);

    return accepted;
}

static const coremodel_spi_func_t verbose_spi_func = {
    .cs = verbose_cs,
    .xfr = verbose_xfr,
};

int main(int argc, char **argv)
{
    const char *target;
    const char *spi_name;
    unsigned csel = 1u;
    void *cm = NULL;
    void *spi = NULL;
    int res;
    ExampleSpiDacActuator dev;

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "usage: %s <address[:port]> <spi-name> [chip-select]\n", argv[0]);
        return 2;
    }

    target = argv[1];
    spi_name = argv[2];
    if (argc == 4) {
        char *endp = NULL;
        unsigned long parsed = strtoul(argv[3], &endp, 0);
        if (!endp || *endp != '\0') {
            fprintf(stderr, "invalid chip-select: %s\n", argv[3]);
            return 2;
        }
        csel = (unsigned)parsed;
    }

    example_spi_dac_actuator_init(&dev);

    res = coremodel_connect(&cm, target);
    if (res) {
        fprintf(stderr, "error: coremodel_connect failed: %s\n", strerror(-res));
        return 1;
    }

    spi = coremodel_attach_spi(cm, spi_name, csel, &verbose_spi_func, &dev, COREMODEL_SPI_BLOCK);
    if (!spi) {
        fprintf(stderr, "error: coremodel_attach_spi(%s, csel=%u) failed\n", spi_name, csel);
        coremodel_disconnect(cm);
        return 1;
    }

    printf("ExampleSpiDacActuator (verbose) attached to SPI %s CS%u.\n", spi_name, csel);
    printf("Protocol: command byte bit7=read, bits6:0=register; 16-bit data is little-endian.\n");
    fflush(stdout);

    res = coremodel_mainloop(cm, -1);

    coremodel_detach(spi);
    coremodel_disconnect(cm);
    return res ? 1 : 0;
}
