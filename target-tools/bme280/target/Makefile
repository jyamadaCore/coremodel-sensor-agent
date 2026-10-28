# Builds bme280_i2c_tool for the imx93 TARGET. Copy this whole directory to
# the imx93 and build there with plain `make`, or cross-compile from your
# host with `make PREFIX=aarch64-linux-gnu-` and copy just the binary over.
PREFIX ?=
CC = $(PREFIX)gcc
CFLAGS = -O2 -Wall -Wextra

bme280_i2c_tool: bme280_i2c_tool.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f bme280_i2c_tool

.PHONY: clean
