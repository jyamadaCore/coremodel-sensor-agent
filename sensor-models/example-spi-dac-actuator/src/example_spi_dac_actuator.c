/*
 * Example SPI Single-Channel DAC Actuator model.
 *
 * CoreModel evidence:
 * - coremodel.h: coremodel_spi_func_t contains cs(void *, unsigned) and
 *   xfr(void *, unsigned, uint8_t *, uint8_t *) callbacks, and SPI attach uses
 *   coremodel_attach_spi(..., csel, ..., COREMODEL_SPI_BLOCK).
 * - examples/spi-ds3234-rtc/spi-ds3234.c: SPI peripheral examples implement a
 *   private state object passed as callback priv and handle register behavior
 *   inside the transfer callback.
 *
 * The synthetic DAC register behavior is intentionally separated from the SPI
 * byte protocol below.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "example_spi_dac_actuator.h"

#include <stddef.h>
#include <string.h>

static bool is_valid_register(uint8_t offset)
{
    switch (offset) {
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL:
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE:
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS:
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_DEVICE_ID:
        return true;
    default:
        return false;
    }
}

void example_spi_dac_actuator_reset(ExampleSpiDacActuator *dev)
{
    if (!dev) {
        return;
    }

    dev->ctrl = 0x0000u;
    dev->dac_code = 0x0000u;
    /* BUSY resolves synchronously and OVERRANGE has no specified triggering
     * condition for the documented 12-bit code range, so both reset/live low. */
    dev->status = 0x0000u;

    dev->spi_selected = false;
    dev->spi_have_cmd = false;
    dev->spi_read = false;
    dev->spi_addr = 0u;
    dev->spi_phase = 0u;
    dev->spi_shift = 0u;
}

void example_spi_dac_actuator_init(ExampleSpiDacActuator *dev)
{
    if (!dev) {
        return;
    }
    memset(dev, 0, sizeof(*dev));
    example_spi_dac_actuator_reset(dev);
}

example_spi_dac_status_t example_spi_dac_actuator_read_reg(const ExampleSpiDacActuator *dev,
                                                           uint8_t offset,
                                                           uint16_t *value)
{
    if (!dev || !value) {
        return EXAMPLE_SPI_DAC_BAD_ARGUMENT;
    }

    switch (offset) {
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL:
        *value = (uint16_t)(dev->ctrl & EXAMPLE_SPI_DAC_ACTUATOR_CTRL_WRITABLE_MASK);
        return EXAMPLE_SPI_DAC_OK;
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE:
        *value = (uint16_t)(dev->dac_code & EXAMPLE_SPI_DAC_ACTUATOR_DAC_CODE_MASK);
        return EXAMPLE_SPI_DAC_OK;
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS:
        *value = dev->status;
        return EXAMPLE_SPI_DAC_OK;
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_DEVICE_ID:
        *value = EXAMPLE_SPI_DAC_ACTUATOR_DEVICE_ID_VALUE;
        return EXAMPLE_SPI_DAC_OK;
    default:
        *value = 0xffffu;
        return EXAMPLE_SPI_DAC_INVALID_REGISTER;
    }
}

example_spi_dac_status_t example_spi_dac_actuator_write_reg(ExampleSpiDacActuator *dev,
                                                            uint8_t offset,
                                                            uint16_t value)
{
    if (!dev) {
        return EXAMPLE_SPI_DAC_BAD_ARGUMENT;
    }
    if (!is_valid_register(offset)) {
        return EXAMPLE_SPI_DAC_INVALID_REGISTER;
    }

    switch (offset) {
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL:
        /* Reserved bits 2:15 are preserved at reset value 0. */
        dev->ctrl = (uint16_t)(value & EXAMPLE_SPI_DAC_ACTUATOR_CTRL_WRITABLE_MASK);
        return EXAMPLE_SPI_DAC_OK;
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE:
        /* CODE is right-justified bits 0:11; reserved bits 12:15 are ignored.
         * The commanded actuator state is observable even when ENABLE is 0. */
        dev->dac_code = (uint16_t)(value & EXAMPLE_SPI_DAC_ACTUATOR_DAC_CODE_MASK);
        dev->status &= (uint16_t)~EXAMPLE_SPI_DAC_ACTUATOR_STATUS_BUSY_MASK;
        return EXAMPLE_SPI_DAC_OK;
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS:
    case EXAMPLE_SPI_DAC_ACTUATOR_REG_DEVICE_ID:
        return EXAMPLE_SPI_DAC_READ_ONLY;
    default:
        return EXAMPLE_SPI_DAC_INVALID_REGISTER;
    }
}

uint16_t example_spi_dac_actuator_get_output_code(const ExampleSpiDacActuator *dev)
{
    return dev ? (uint16_t)(dev->dac_code & EXAMPLE_SPI_DAC_ACTUATOR_DAC_CODE_MASK) : 0u;
}

bool example_spi_dac_actuator_get_output_enabled(const ExampleSpiDacActuator *dev)
{
    return dev ? ((dev->ctrl & EXAMPLE_SPI_DAC_ACTUATOR_CTRL_ENABLE_MASK) != 0u) : false;
}

bool example_spi_dac_actuator_get_output_driven(const ExampleSpiDacActuator *dev)
{
    return example_spi_dac_actuator_get_output_enabled(dev);
}

bool example_spi_dac_actuator_get_range_sel(const ExampleSpiDacActuator *dev)
{
    return dev ? ((dev->ctrl & EXAMPLE_SPI_DAC_ACTUATOR_CTRL_RANGE_SEL_MASK) != 0u) : false;
}

unsigned example_spi_dac_actuator_get_interrupt_count(const ExampleSpiDacActuator *dev)
{
    (void)dev;
    return 0u;
}

bool example_spi_dac_actuator_get_irq_active(const ExampleSpiDacActuator *dev)
{
    (void)dev;
    return false;
}

void example_spi_dac_actuator_spi_cs(void *priv, unsigned csel)
{
    ExampleSpiDacActuator *dev = (ExampleSpiDacActuator *)priv;
    if (!dev) {
        return;
    }

    dev->spi_selected = (csel != 0u);
    dev->spi_have_cmd = false;
    dev->spi_read = false;
    dev->spi_addr = 0u;
    dev->spi_phase = 0u;
    dev->spi_shift = 0u;
}

static uint8_t spi_next_read_byte(ExampleSpiDacActuator *dev)
{
    uint8_t out;

    if (dev->spi_phase == 0u) {
        out = (uint8_t)(dev->spi_shift & 0xffu);
        dev->spi_phase = 1u;
        return out;
    }

    out = (uint8_t)((dev->spi_shift >> 8) & 0xffu);
    dev->spi_have_cmd = false;
    dev->spi_phase = 0u;
    return out;
}

static void spi_accept_command(ExampleSpiDacActuator *dev, uint8_t cmd)
{
    uint16_t value;

    dev->spi_have_cmd = true;
    dev->spi_read = ((cmd & EXAMPLE_SPI_DAC_ACTUATOR_SPI_READ_FLAG) != 0u);
    dev->spi_addr = (uint8_t)(cmd & EXAMPLE_SPI_DAC_ACTUATOR_SPI_ADDR_MASK);
    dev->spi_phase = 0u;
    dev->spi_shift = 0u;

    if (dev->spi_read) {
        if (example_spi_dac_actuator_read_reg(dev, dev->spi_addr, &value) == EXAMPLE_SPI_DAC_OK) {
            dev->spi_shift = value;
        } else {
            dev->spi_shift = 0xffffu;
        }
    }
}

int example_spi_dac_actuator_spi_xfr(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata)
{
    ExampleSpiDacActuator *dev = (ExampleSpiDacActuator *)priv;
    unsigned i;

    if (!dev || (!wrdata && len != 0u) || !rddata) {
        return 0;
    }

    for (i = 0u; i < len; i++) {
        uint8_t in = wrdata[i];
        rddata[i] = 0x00u;

        if (!dev->spi_have_cmd) {
            spi_accept_command(dev, in);
            continue;
        }

        if (dev->spi_read) {
            rddata[i] = spi_next_read_byte(dev);
            continue;
        }

        if (dev->spi_phase == 0u) {
            dev->spi_shift = in;
            dev->spi_phase = 1u;
        } else {
            dev->spi_shift |= (uint16_t)((uint16_t)in << 8);
            (void)example_spi_dac_actuator_write_reg(dev, dev->spi_addr, dev->spi_shift);
            dev->spi_have_cmd = false;
            dev->spi_phase = 0u;
            dev->spi_shift = 0u;
        }
    }

    return (int)len;
}
