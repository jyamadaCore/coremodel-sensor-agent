/* ADXL345 register behavior and SPI framing. CoreModel binding is separate in
 * adxl345_coremodel.c. Evidence paths are listed in GENERATION_REPORT.md. */
#include "adxl345_model.h"
#include <string.h>
void adxl345_reset(ADXL345Model *dev){ if(!dev)return; memset(dev,0,sizeof(*dev)); dev->regs[ADXL345_REG_DEVID]=ADXL345_DEVID_RESET; }
bool adxl345_reg_is_valid(uint8_t a){ switch(a){case ADXL345_REG_DEVID:case ADXL345_REG_POWER_CTL:case ADXL345_REG_DATAX0:case ADXL345_REG_DATAX1:case ADXL345_REG_DATAY0:case ADXL345_REG_DATAY1:case ADXL345_REG_DATAZ0:case ADXL345_REG_DATAZ1:return true;default:return false;} }
bool adxl345_reg_is_readable(uint8_t a){ return adxl345_reg_is_valid(a); }
bool adxl345_reg_is_writable(uint8_t a){ return a==ADXL345_REG_POWER_CTL; }
const char *adxl345_reg_name(uint8_t a){ switch(a){case ADXL345_REG_DEVID:return "DEVID";case ADXL345_REG_POWER_CTL:return "POWER_CTL";case ADXL345_REG_DATAX0:return "DATAX0";case ADXL345_REG_DATAX1:return "DATAX1";case ADXL345_REG_DATAY0:return "DATAY0";case ADXL345_REG_DATAY1:return "DATAY1";case ADXL345_REG_DATAZ0:return "DATAZ0";case ADXL345_REG_DATAZ1:return "DATAZ1";default:return "INVALID";} }
uint8_t adxl345_reg_read(ADXL345Model *dev,uint8_t a){ if(!dev||!adxl345_reg_is_readable(a))return 0; return dev->regs[a]; }
bool adxl345_reg_write(ADXL345Model *dev,uint8_t a,uint8_t v){ if(!dev||!adxl345_reg_is_writable(a))return false; dev->regs[ADXL345_REG_POWER_CTL]=v&ADXL345_POWER_MEASURE; return true; }
void adxl345_set_acceleration(ADXL345Model *d,int16_t x,int16_t y,int16_t z){ if(!d)return; d->regs[ADXL345_REG_DATAX0]=(uint8_t)((uint16_t)x); d->regs[ADXL345_REG_DATAX1]=(uint8_t)(((uint16_t)x)>>8); d->regs[ADXL345_REG_DATAY0]=(uint8_t)((uint16_t)y); d->regs[ADXL345_REG_DATAY1]=(uint8_t)(((uint16_t)y)>>8); d->regs[ADXL345_REG_DATAZ0]=(uint8_t)((uint16_t)z); d->regs[ADXL345_REG_DATAZ1]=(uint8_t)(((uint16_t)z)>>8); }
bool adxl345_measure_enabled(const ADXL345Model *d){ return d&&((d->regs[ADXL345_REG_POWER_CTL]&ADXL345_POWER_MEASURE)!=0); }
bool adxl345_interrupt_pending(const ADXL345Model *d){ return d&&d->interrupt_pending; }
void adxl345_spi_cs(ADXL345Model *d,unsigned asserted){ if(!d)return; if(asserted){ d->spi_have_command=false; d->spi_addr=0; d->spi_read=false; d->spi_multibyte=false; } }
static void adv(ADXL345Model*d){ if(d->spi_multibyte)d->spi_addr=(uint8_t)((d->spi_addr+1u)&ADXL345_SPI_ADDR_MASK); }
int adxl345_spi_xfr(ADXL345Model*d,unsigned len,const uint8_t*wr,uint8_t*rd){ if(!d||!wr||!rd)return 0; for(unsigned i=0;i<len;i++){ uint8_t b=wr[i]; if(!d->spi_have_command){ d->spi_read=(b&ADXL345_SPI_READ)!=0; d->spi_multibyte=(b&ADXL345_SPI_MULTIBYTE)!=0; d->spi_addr=b&ADXL345_SPI_ADDR_MASK; d->spi_have_command=true; rd[i]=0; } else { if(d->spi_read)rd[i]=adxl345_reg_read(d,d->spi_addr); else { (void)adxl345_reg_write(d,d->spi_addr,b); rd[i]=0; } adv(d); } } return (int)len; }
