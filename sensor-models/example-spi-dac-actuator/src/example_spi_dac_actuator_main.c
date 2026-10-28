/*
 * CoreModel SPI attachment executable for ExampleSpiDacActuator.
 * Evidence for API use: coremodel.h and examples/spi-ds3234-rtc/spi-ds3234.c.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "example_spi_dac_actuator.h"

#include <coremodel.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const coremodel_spi_func_t example_spi_dac_actuator_spi_func = {
    .cs = example_spi_dac_actuator_spi_cs,
    .xfr = example_spi_dac_actuator_spi_xfr,
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

    spi = coremodel_attach_spi(cm, spi_name, csel, &example_spi_dac_actuator_spi_func,
                               &dev, COREMODEL_SPI_BLOCK);
    if (!spi) {
        fprintf(stderr, "error: coremodel_attach_spi(%s, csel=%u) failed\n", spi_name, csel);
        coremodel_disconnect(cm);
        return 1;
    }

    printf("ExampleSpiDacActuator attached to SPI %s CS%u.\n", spi_name, csel);
    printf("Protocol: command byte bit7=read, bits6:0=register; 16-bit data is little-endian.\n");
    fflush(stdout);

    res = coremodel_mainloop(cm, -1);

    coremodel_detach(spi);
    coremodel_disconnect(cm);
    return res ? 1 : 0;
}
