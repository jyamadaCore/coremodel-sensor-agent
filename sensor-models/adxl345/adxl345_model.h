/* Generated ADXL345 register model. Evidence: CoreModel README.md, coremodel.h,
 * examples/spi-ds3234-rtc/spi-ds3234.c, examples/i2c-adxl36x/adxl36x_registers.c. */
#ifndef ADXL345_MODEL_H
#define ADXL345_MODEL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define ADXL345_REG_DEVID 0x00u
#define ADXL345_REG_POWER_CTL 0x2du
#define ADXL345_REG_DATAX0 0x32u
#define ADXL345_REG_DATAX1 0x33u
#define ADXL345_REG_DATAY0 0x34u
#define ADXL345_REG_DATAY1 0x35u
#define ADXL345_REG_DATAZ0 0x36u
#define ADXL345_REG_DATAZ1 0x37u
#define ADXL345_DEVID_RESET 0xe5u
#define ADXL345_POWER_MEASURE 0x08u
#define ADXL345_SPI_READ 0x80u
#define ADXL345_SPI_MULTIBYTE 0x40u
#define ADXL345_SPI_ADDR_MASK 0x3fu
typedef struct ADXL345Model { uint8_t regs[256]; uint8_t spi_addr; bool spi_have_command; bool spi_read; bool spi_multibyte; bool interrupt_pending; } ADXL345Model;
void adxl345_reset(ADXL345Model *dev);
uint8_t adxl345_reg_read(ADXL345Model *dev, uint8_t addr);
bool adxl345_reg_write(ADXL345Model *dev, uint8_t addr, uint8_t value);
bool adxl345_reg_is_valid(uint8_t addr);
bool adxl345_reg_is_writable(uint8_t addr);
bool adxl345_reg_is_readable(uint8_t addr);
void adxl345_set_acceleration(ADXL345Model *dev, int16_t x, int16_t y, int16_t z);
bool adxl345_measure_enabled(const ADXL345Model *dev);
bool adxl345_interrupt_pending(const ADXL345Model *dev);
void adxl345_spi_cs(ADXL345Model *dev, unsigned asserted);
int adxl345_spi_xfr(ADXL345Model *dev, unsigned len, const uint8_t *wrdata, uint8_t *rddata);
const char *adxl345_reg_name(uint8_t addr);
#endif
