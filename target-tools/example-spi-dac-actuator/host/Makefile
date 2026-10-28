# Builds run-example-spi-dac-actuator-verbose for your HOST machine (same
# place you already run run-example-spi-dac-actuator). This is the sensor
# side: it attaches to CoreModel and traces every byte. Never deploy or
# build this on the imx93 target.
CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -O2 -g

run-example-spi-dac-actuator-verbose: run-example-spi-dac-actuator-verbose.c \
		example_spi_dac_actuator.c example_spi_dac_actuator.h coremodel.o
	$(CC) $(CFLAGS) -o $@ run-example-spi-dac-actuator-verbose.c \
		example_spi_dac_actuator.c coremodel.o -lpthread

coremodel.o: coremodel.c coremodel.h
	$(CC) -std=c11 -Wall -c -o $@ coremodel.c

clean:
	rm -f run-example-spi-dac-actuator-verbose coremodel.o

.PHONY: clean
