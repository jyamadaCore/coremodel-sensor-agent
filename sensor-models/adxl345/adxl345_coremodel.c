/* CoreModel SPI adapter. Grounded in coremodel.h and README.md SPI API plus
 * examples/spi-ds3234-rtc/spi-ds3234.c callback-state pattern. */
#include "adxl345_model.h"
#include <coremodel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void cm_cs(void *priv,unsigned csel){ adxl345_spi_cs((ADXL345Model*)priv,csel); }
static int cm_xfr(void *priv,unsigned len,uint8_t *wr,uint8_t *rd){ return adxl345_spi_xfr((ADXL345Model*)priv,len,wr,rd); }
static const coremodel_spi_func_t funcs={ .cs=cm_cs, .xfr=cm_xfr };
static void usage(const char*p){ fprintf(stderr,"usage: %s <address[:port]> <spi-bus-name> [chip-select]\n",p); }
int main(int argc,char**argv){ void *cm=NULL,*spi=NULL; ADXL345Model dev; unsigned csel=0; int res; if(argc<3||argc>4){usage(argv[0]);return 2;} if(argc==4)csel=(unsigned)strtoul(argv[3],NULL,0); adxl345_reset(&dev); res=coremodel_connect(&cm,argv[1]); if(res){fprintf(stderr,"ADXL345: connect failed: %s\n",strerror(-res));return 1;} spi=coremodel_attach_spi(cm,argv[2],csel,&funcs,&dev,COREMODEL_SPI_BLOCK); if(!spi){fprintf(stderr,"ADXL345: SPI attach failed\n"); coremodel_disconnect(cm); return 1;} printf("ADXL345 attached to %s CS%u\n",argv[2],csel); fflush(stdout); res=coremodel_mainloop(cm,-1); coremodel_detach(spi); coremodel_disconnect(cm); return res?1:0; }
