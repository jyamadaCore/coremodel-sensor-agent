/*
 * Example SPI Single-Channel DAC Actuator CoreModel support model.
 *
 * Evidence: CoreModel SPI callback and attach ABI follows coremodel.h and
 * examples/spi-ds3234-rtc/spi-ds3234.c from the inspected CoreModel tree.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef EXAMPLE_SPI_DAC_ACTUATOR_H
#define EXAMPLE_SPI_DAC_ACTUATOR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL       0x00u
#define EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE   0x01u
#define EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS     0x02u
#define EXAMPLE_SPI_DAC_ACTUATOR_REG_DEVICE_ID  0x0fu

#define EXAMPLE_SPI_DAC_ACTUATOR_CTRL_ENABLE_MASK     0x0001u
#define EXAMPLE_SPI_DAC_ACTUATOR_CTRL_RANGE_SEL_MASK  0x0002u
#define EXAMPLE_SPI_DAC_ACTUATOR_CTRL_WRITABLE_MASK   0x0003u
#define EXAMPLE_SPI_DAC_ACTUATOR_DAC_CODE_MASK        0x0fffu
#define EXAMPLE_SPI_DAC_ACTUATOR_STATUS_BUSY_MASK     0x0001u
#define EXAMPLE_SPI_DAC_ACTUATOR_STATUS_OVERRANGE_MASK 0x0002u
#define EXAMPLE_SPI_DAC_ACTUATOR_DEVICE_ID_VALUE      0x5ac1u

#define EXAMPLE_SPI_DAC_ACTUATOR_SPI_READ_FLAG        0x80u
#define EXAMPLE_SPI_DAC_ACTUATOR_SPI_ADDR_MASK        0x7fu

typedef enum example_spi_dac_status {
    EXAMPLE_SPI_DAC_OK = 0,
    EXAMPLE_SPI_DAC_INVALID_REGISTER = -1,
    EXAMPLE_SPI_DAC_READ_ONLY = -2,
    EXAMPLE_SPI_DAC_BAD_ARGUMENT = -3
} example_spi_dac_status_t;

typedef struct ExampleSpiDacActuator {
    uint16_t ctrl;
    uint16_t dac_code;
    uint16_t status;

    /* SPI transport state. Register behavior above remains transport-neutral. */
    bool spi_selected;
    bool spi_have_cmd;
    bool spi_read;
    uint8_t spi_addr;
    uint8_t spi_phase;
    uint16_t spi_shift;
} ExampleSpiDacActuator;

void example_spi_dac_actuator_init(ExampleSpiDacActuator *dev);
void example_spi_dac_actuator_reset(ExampleSpiDacActuator *dev);

example_spi_dac_status_t example_spi_dac_actuator_read_reg(const ExampleSpiDacActuator *dev,
                                                           uint8_t offset,
                                                           uint16_t *value);
example_spi_dac_status_t example_spi_dac_actuator_write_reg(ExampleSpiDacActuator *dev,
                                                            uint8_t offset,
                                                            uint16_t value);

uint16_t example_spi_dac_actuator_get_output_code(const ExampleSpiDacActuator *dev);
bool example_spi_dac_actuator_get_output_enabled(const ExampleSpiDacActuator *dev);
bool example_spi_dac_actuator_get_output_driven(const ExampleSpiDacActuator *dev);
bool example_spi_dac_actuator_get_range_sel(const ExampleSpiDacActuator *dev);
unsigned example_spi_dac_actuator_get_interrupt_count(const ExampleSpiDacActuator *dev);
bool example_spi_dac_actuator_get_irq_active(const ExampleSpiDacActuator *dev);

void example_spi_dac_actuator_spi_cs(void *priv, unsigned csel);
int example_spi_dac_actuator_spi_xfr(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata);

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLE_SPI_DAC_ACTUATOR_H */
