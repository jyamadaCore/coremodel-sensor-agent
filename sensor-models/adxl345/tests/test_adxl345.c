#include "../adxl345_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define T(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define E(e,g) do{unsigned ee=(unsigned)(uint8_t)(e),gg=(unsigned)(uint8_t)(g);if(ee!=gg){fprintf(stderr,"FAIL %s:%d exp=%02x got=%02x\n",__FILE__,__LINE__,ee,gg);exit(1);}}while(0)
static uint8_t sr(ADXL345Model*d,uint8_t a){uint8_t tx[2]={(uint8_t)(ADXL345_SPI_READ|(a&ADXL345_SPI_ADDR_MASK)),0},rx[2]={0};adxl345_spi_cs(d,1);T(adxl345_spi_xfr(d,2,tx,rx)==2);return rx[1];}
static void sw(ADXL345Model*d,uint8_t a,uint8_t v){uint8_t tx[2]={(uint8_t)(a&ADXL345_SPI_ADDR_MASK),v},rx[2]={0};adxl345_spi_cs(d,1);T(adxl345_spi_xfr(d,2,tx,rx)==2);}
int main(void){ADXL345Model d; uint8_t tx[7],rx[7]; adxl345_reset(&d); E(0xe5,adxl345_reg_read(&d,ADXL345_REG_DEVID)); E(0,adxl345_reg_read(&d,ADXL345_REG_POWER_CTL)); E(0,adxl345_reg_read(&d,ADXL345_REG_DATAX0)); T(!adxl345_measure_enabled(&d)); T(!adxl345_interrupt_pending(&d));
T(!adxl345_reg_write(&d,ADXL345_REG_DEVID,0)); E(0xe5,adxl345_reg_read(&d,ADXL345_REG_DEVID)); adxl345_set_acceleration(&d,0x1234,-2,0x7f00); T(!adxl345_reg_write(&d,ADXL345_REG_DATAX0,0xaa)); E(0x34,adxl345_reg_read(&d,ADXL345_REG_DATAX0)); E(0xe5,adxl345_reg_read(&d,ADXL345_REG_DEVID)); E(0xe5,adxl345_reg_read(&d,ADXL345_REG_DEVID));
T(adxl345_reg_write(&d,ADXL345_REG_POWER_CTL,0xff)); E(ADXL345_POWER_MEASURE,adxl345_reg_read(&d,ADXL345_REG_POWER_CTL)); T(adxl345_measure_enabled(&d)); T(adxl345_reg_write(&d,ADXL345_REG_POWER_CTL,0)); E(0,adxl345_reg_read(&d,ADXL345_REG_POWER_CTL));
adxl345_set_acceleration(&d,(int16_t)0x1234,(int16_t)0xabcd,(int16_t)-1); E(0x34,adxl345_reg_read(&d,ADXL345_REG_DATAX0)); E(0x12,adxl345_reg_read(&d,ADXL345_REG_DATAX1)); E(0xcd,adxl345_reg_read(&d,ADXL345_REG_DATAY0)); E(0xab,adxl345_reg_read(&d,ADXL345_REG_DATAY1)); E(0xff,adxl345_reg_read(&d,ADXL345_REG_DATAZ0)); E(0xff,adxl345_reg_read(&d,ADXL345_REG_DATAZ1));
E(0xe5,sr(&d,ADXL345_REG_DEVID)); sw(&d,ADXL345_REG_POWER_CTL,0xff); E(ADXL345_POWER_MEASURE,sr(&d,ADXL345_REG_POWER_CTL)); adxl345_set_acceleration(&d,0x0102,0x0304,0x0506); memset(tx,0,sizeof(tx)); memset(rx,0,sizeof(rx)); tx[0]=ADXL345_SPI_READ|ADXL345_SPI_MULTIBYTE|ADXL345_REG_DATAX0; adxl345_spi_cs(&d,1); T(adxl345_spi_xfr(&d,sizeof(tx),tx,rx)==(int)sizeof(tx)); E(2,rx[1]); E(1,rx[2]); E(4,rx[3]); E(3,rx[4]); E(6,rx[5]); E(5,rx[6]);
T(!adxl345_reg_is_valid(0x10)); E(0,adxl345_reg_read(&d,0x10)); T(!adxl345_reg_write(&d,0x10,0xff)); E(0,sr(&d,0x10)); T(!adxl345_interrupt_pending(&d)); puts("ADXL345Model tests passed"); return 0;}
