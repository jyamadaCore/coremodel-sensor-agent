# Builds spi_dac_tool for the imx93 TARGET. Copy this whole directory to
# the imx93 and build there with plain `make`, or cross-compile from your
# host with `make PREFIX=aarch64-linux-gnu-` and copy just the binary over.
PREFIX ?=
CC = $(PREFIX)gcc
CFLAGS = -O2 -Wall -Wextra

spi_dac_tool: spi_dac_tool.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f spi_dac_tool

.PHONY: clean
