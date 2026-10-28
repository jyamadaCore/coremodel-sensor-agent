/* CoreModel executable binding for generated BME280 model.
 * Uses API pattern from examples/i2c/coremodel-i2c.c and examples/i2c-ds3231-rtc/i2c-ds3231.c.
 */
#include "bme280_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <coremodel.h>

int main(int argc, char **argv)
{
    void *cm = NULL;
    void *handle = NULL;
    int res;
    BME280Model *state;

    if(argc != 3) {
        fprintf(stderr, "usage: %s <address[:port]> <i2c-bus>\n", argv[0]);
        return 1;
    }

    state = calloc(1, sizeof(*state));
    if(state == NULL) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }
    bme280_init(state);

    res = coremodel_connect(&cm, argv[1]);
    if(res != 0) {
        fprintf(stderr, "error: failed to connect: %s\n", strerror(-res));
        free(state);
        return 1;
    }

    handle = coremodel_attach_i2c(cm, argv[2], BME280_I2C_ADDRESS,
                                  bme280_coremodel_i2c_func(), state, 0);
    if(handle == NULL) {
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
