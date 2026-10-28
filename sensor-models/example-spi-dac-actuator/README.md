# Example SPI Single-Channel DAC Actuator

This directory contains a generated CoreModel-compatible SPI peripheral model for the synthetic **Example SPI Single-Channel DAC Actuator**.

## CoreModel binding evidence

The SPI binding uses only APIs observed in the inspected CoreModel library:

- `coremodel.h`: `coremodel_spi_func_t`, `coremodel_attach_spi`, `COREMODEL_SPI_BLOCK`, `coremodel_mainloop`, `coremodel_detach`, and `coremodel_disconnect`.
- `examples/spi-ds3234-rtc/spi-ds3234.c`: private device state is passed as callback `priv`, with register behavior implemented from SPI callbacks.
- `README.md`: documents CoreModel connection, list, attach, and main-loop conventions.

## Register model

Registers are addressed by 8-bit offsets and contain 16-bit little-endian data:

| Offset | Name | Access | Reset | Notes |
|---:|---|---|---:|---|
| `0x00` | `CTRL` | RW | `0x0000` | bit0 `ENABLE`, bit1 `RANGE_SEL`; bits 2:15 are reserved and forced to 0. |
| `0x01` | `DAC_CODE` | RW | `0x0000` | bits 0:11 are the commanded DAC code; bits 12:15 are reserved and forced to 0. |
| `0x02` | `STATUS` | RO | `0x0000` | `BUSY` and `OVERRANGE`; BUSY is synchronous/instantaneous and remains 0. |
| `0x0F` | `DEVICE_ID` | RO | `0x5AC1` | fixed identity value. |

The actuator output is observable through:

- `example_spi_dac_actuator_get_output_code()`
- `example_spi_dac_actuator_get_output_enabled()`
- `example_spi_dac_actuator_get_output_driven()`
- `example_spi_dac_actuator_get_range_sel()`

A `DAC_CODE` write updates the commanded code even when `ENABLE` is clear. `get_output_driven()` is true only when `ENABLE` is set.

## SPI transport assumption

The user specification defined 8-bit register addresses and 16-bit register data but did not define an SPI opcode format. The generated model therefore uses this documented, test-covered framing:

- Command byte: bit 7 is read (`1`) / write (`0`), bits 6:0 are the register address.
- Data: two bytes, little-endian, following the command byte.
- Read response: the command byte clocks out `0x00`, then low byte, then high byte.
- Invalid register reads return `0xFFFF`; invalid writes are ignored by register behavior.

Transport framing is isolated in `example_spi_dac_actuator_spi_xfr()`; register behavior is available independently through `read_reg` / `write_reg`.

## Build and test

```sh
make test
```

If `COREMODEL_DIR` points to an inspected CoreModel checkout containing `coremodel.h` and `coremodel.c`, `make` also builds the attach executable:

```sh
make COREMODEL_DIR=/path/to/coremodel
./run-example-spi-dac-actuator 127.0.0.1:1900 <spi-name> 1
```

The default chip select from the normalized specification is `1`.

## Unsupported behavior

- No physical analog voltage, settling waveform, load model, or range-dependent voltage calculation is invented.
- No interrupts are implemented because the specification lists no interrupts.
- `OVERRANGE` has no trigger for documented 12-bit codes; reserved high bits are masked rather than treated as commanded analog values.
