#include "example_spi_dac_actuator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "ASSERT_TRUE failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

#define ASSERT_EQ_U16(exp, got) do { \
    uint16_t _e = (uint16_t)(exp); \
    uint16_t _g = (uint16_t)(got); \
    if (_e != _g) { \
        fprintf(stderr, "ASSERT_EQ_U16 failed at %s:%d: expected 0x%04x got 0x%04x\n", \
                __FILE__, __LINE__, _e, _g); \
        exit(1); \
    } \
} while (0)

#define ASSERT_EQ_INT(exp, got) do { \
    int _e = (int)(exp); \
    int _g = (int)(got); \
    if (_e != _g) { \
        fprintf(stderr, "ASSERT_EQ_INT failed at %s:%d: expected %d got %d\n", \
                __FILE__, __LINE__, _e, _g); \
        exit(1); \
    } \
} while (0)

static uint16_t read_reg_checked(const ExampleSpiDacActuator *dev, uint8_t reg)
{
    uint16_t value = 0xdeadu;
    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_OK, example_spi_dac_actuator_read_reg(dev, reg, &value));
    return value;
}

static void test_reset_values(void)
{
    ExampleSpiDacActuator dev;
    example_spi_dac_actuator_init(&dev);

    ASSERT_EQ_U16(0x0000u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL));
    ASSERT_EQ_U16(0x0000u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE));
    ASSERT_EQ_U16(0x0000u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS));
    ASSERT_EQ_U16(0x5ac1u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DEVICE_ID));
    ASSERT_EQ_U16(0x0000u, example_spi_dac_actuator_get_output_code(&dev));
    ASSERT_TRUE(!example_spi_dac_actuator_get_output_enabled(&dev));
    ASSERT_TRUE(!example_spi_dac_actuator_get_output_driven(&dev));
}

static void test_read_write_access_and_field_masks(void)
{
    ExampleSpiDacActuator dev;
    example_spi_dac_actuator_init(&dev);

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_OK,
                  example_spi_dac_actuator_write_reg(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL, 0xffffu));
    ASSERT_EQ_U16(0x0003u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL));
    ASSERT_TRUE(example_spi_dac_actuator_get_output_enabled(&dev));
    ASSERT_TRUE(example_spi_dac_actuator_get_range_sel(&dev));

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_OK,
                  example_spi_dac_actuator_write_reg(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE, 0xffffu));
    ASSERT_EQ_U16(0x0fffu, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE));
    ASSERT_EQ_U16(0x0fffu, example_spi_dac_actuator_get_output_code(&dev));

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_READ_ONLY,
                  example_spi_dac_actuator_write_reg(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS, 0x0003u));
    ASSERT_EQ_U16(0x0000u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS));

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_READ_ONLY,
                  example_spi_dac_actuator_write_reg(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DEVICE_ID, 0x0000u));
    ASSERT_EQ_U16(0x5ac1u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DEVICE_ID));
}

static void test_side_effects_and_actuator_getters(void)
{
    ExampleSpiDacActuator dev;
    example_spi_dac_actuator_init(&dev);

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_OK,
                  example_spi_dac_actuator_write_reg(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE, 0x0123u));
    ASSERT_EQ_U16(0x0123u, example_spi_dac_actuator_get_output_code(&dev));
    ASSERT_TRUE(!example_spi_dac_actuator_get_output_enabled(&dev));
    ASSERT_TRUE(!example_spi_dac_actuator_get_output_driven(&dev));
    ASSERT_EQ_U16(0x0000u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_STATUS));

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_OK,
                  example_spi_dac_actuator_write_reg(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL,
                                                     EXAMPLE_SPI_DAC_ACTUATOR_CTRL_ENABLE_MASK));
    ASSERT_TRUE(example_spi_dac_actuator_get_output_enabled(&dev));
    ASSERT_TRUE(example_spi_dac_actuator_get_output_driven(&dev));
    ASSERT_EQ_U16(0x0123u, example_spi_dac_actuator_get_output_code(&dev));
}

static void test_interrupts_absent(void)
{
    ExampleSpiDacActuator dev;
    example_spi_dac_actuator_init(&dev);

    ASSERT_EQ_INT(0, (int)example_spi_dac_actuator_get_interrupt_count(&dev));
    ASSERT_TRUE(!example_spi_dac_actuator_get_irq_active(&dev));
}

static void test_invalid_accesses(void)
{
    ExampleSpiDacActuator dev;
    uint16_t value = 0u;
    example_spi_dac_actuator_init(&dev);

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_INVALID_REGISTER,
                  example_spi_dac_actuator_read_reg(&dev, 0x03u, &value));
    ASSERT_EQ_U16(0xffffu, value);
    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_INVALID_REGISTER,
                  example_spi_dac_actuator_write_reg(&dev, 0x03u, 0x1111u));
    ASSERT_EQ_U16(0x0000u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL));

    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_BAD_ARGUMENT,
                  example_spi_dac_actuator_read_reg(NULL, EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL, &value));
    ASSERT_EQ_INT(EXAMPLE_SPI_DAC_BAD_ARGUMENT,
                  example_spi_dac_actuator_write_reg(NULL, EXAMPLE_SPI_DAC_ACTUATOR_REG_CTRL, 0u));
}

static void test_spi_framing(void)
{
    ExampleSpiDacActuator dev;
    uint8_t wr[3];
    uint8_t rd[3];
    example_spi_dac_actuator_init(&dev);

    example_spi_dac_actuator_spi_cs(&dev, 1u);
    wr[0] = EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE;
    wr[1] = 0x34u;
    wr[2] = 0x12u;
    memset(rd, 0xa5, sizeof(rd));
    ASSERT_EQ_INT(3, example_spi_dac_actuator_spi_xfr(&dev, sizeof(wr), wr, rd));
    ASSERT_EQ_U16(0x0234u, read_reg_checked(&dev, EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE));

    example_spi_dac_actuator_spi_cs(&dev, 1u);
    wr[0] = (uint8_t)(EXAMPLE_SPI_DAC_ACTUATOR_SPI_READ_FLAG | EXAMPLE_SPI_DAC_ACTUATOR_REG_DAC_CODE);
    wr[1] = 0x00u;
    wr[2] = 0x00u;
    memset(rd, 0, sizeof(rd));
    ASSERT_EQ_INT(3, example_spi_dac_actuator_spi_xfr(&dev, sizeof(wr), wr, rd));
    ASSERT_EQ_U16(0x0000u, rd[0]);
    ASSERT_EQ_U16(0x0034u, rd[1]);
    ASSERT_EQ_U16(0x0002u, rd[2]);

    example_spi_dac_actuator_spi_cs(&dev, 1u);
    wr[0] = (uint8_t)(EXAMPLE_SPI_DAC_ACTUATOR_SPI_READ_FLAG | 0x03u);
    wr[1] = 0x00u;
    wr[2] = 0x00u;
    memset(rd, 0, sizeof(rd));
    ASSERT_EQ_INT(3, example_spi_dac_actuator_spi_xfr(&dev, sizeof(wr), wr, rd));
    ASSERT_EQ_U16(0x00ffu, rd[1]);
    ASSERT_EQ_U16(0x00ffu, rd[2]);
}

int main(void)
{
    test_reset_values();
    test_read_write_access_and_field_masks();
    test_side_effects_and_actuator_getters();
    test_interrupts_absent();
    test_invalid_accesses();
    test_spi_framing();
    puts("all ExampleSpiDacActuator tests passed");
    return 0;
}
