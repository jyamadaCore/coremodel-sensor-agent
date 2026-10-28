# BME280 CoreModel Sensor Model

Generated I2C model for Bosch BME280 at 7-bit address `0x76`.

## Build

```sh
make test
```

`Makefile` and `CMakeLists.txt` compile generated code with `-Wall -Wextra -Werror`; the vendored CoreModel `coremodel.c` is compiled as a separate object with lenient `-Wall` only, matching the generation rules.

## CoreModel usage

```sh
./bme280-coremodel 127.0.0.1:1900 <i2c-bus-name>
```

The binding uses `coremodel_connect`, `coremodel_attach_i2c`, `coremodel_mainloop`, `coremodel_detach`, and `coremodel_disconnect` exactly as shown by the inspected CoreModel I2C examples.

## Modeled behavior

- 8-bit register-addressed I2C slave.
- Register reads auto-increment.
- Writes consume register/data pairs after the first register-address byte; this follows the supplied normalized timing note that multi-byte writes are not auto-incremented.
- Chip ID register `0xD0` reads `0x60`.
- Factory calibration ranges `0x88..0xA1` and `0xE1..0xE7` are read-only. Values default to the schema placeholder `0x00` and can be configured with `bme280_set_calibration_byte()`.
- Sensor injection APIs expose host-provided raw ADC readings:
  - `bme280_set_raw_pressure()` for unsigned 20-bit pressure.
  - `bme280_set_raw_temperature()` for unsigned 20-bit temperature.
  - `bme280_set_raw_humidity()` for unsigned 16-bit humidity.
- Soft reset command `0xB6` at register `0xE0` resets control/status/output registers and preserves calibration bytes.
- No interrupt output is modeled because none was specified.

## Unsupported / intentionally limited

- No physical compensation, filtering, timing, oscillator, voltage, self-heating, or environmental physics is implemented.
- Alternate SPI mode and CSB/SDO pin electrical state are documented only; the selected CoreModel transport is I2C.
- Conversion timing is completed synchronously for forced-mode writes; status timing bits remain clear outside externally injected state.
