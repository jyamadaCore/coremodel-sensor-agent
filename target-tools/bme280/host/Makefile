# Builds bme280_coremodel_inject for your HOST machine (same place you'd
# run sensor-models/bme280/bme280-coremodel). This is the sensor side: it
# attaches to CoreModel with synthetic calibration + raw ADC data already
# loaded. Never deploy or build this on the imx93 target.
CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -O2 -g -I.

bme280_coremodel_inject: bme280_coremodel_inject.c bme280_model.c bme280_model.h coremodel.o
	$(CC) $(CFLAGS) -o $@ bme280_coremodel_inject.c bme280_model.c coremodel.o -lpthread

coremodel.o: coremodel.c coremodel.h
	$(CC) -std=c11 -Wall -I. -c -o $@ coremodel.c

clean:
	rm -f bme280_coremodel_inject coremodel.o

.PHONY: clean
