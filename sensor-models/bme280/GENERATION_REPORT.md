# Generation Report: BME280

## Validation

`validateSensorSpecTool` returned `valid=true`.

Warnings preserved:
- CAN dataBitrate is ignored unless protocol is can-fd.
- No obvious identity/version register was found; the supplied map does include BME280 ID register `0xD0` with reset value `0x60`.

## CoreModel evidence

Inspected source paths used as API authority:
- `README.md` — CoreModel connection, main loop, I2C callback, attach, ready semantics.
- `coremodel.h` — declarations for `coremodel_i2c_func_t`, `coremodel_attach_i2c`, flags, detach/disconnect.
- `coremodel.c` — implementation confirms I2C packet/callback semantics.
- `examples/i2c/coremodel-i2c.c` — minimal I2C model attach/mainloop pattern.
- `examples/i2c-ds3231-rtc/i2c-ds3231.c` — register-indexed I2C peripheral pattern with auto-incremented reads.
- `examples/i2c-adxl36x/coremodel-adxl36x.c` and `.h` — larger register model and generated field/mask style.

## Assumptions

- Calibration reset values are placeholders from the normalized schema; runtime calibration injection is provided.
- Invalid/reserved register reads return `0x00`; invalid writes are ignored.
- Read-only registers ignore writes.
- Reserved bits in writable registers are preserved/masked: `ctrl_hum` writes only bits `[2:0]`; `config` preserves bit 1 as reserved zero.
- Forced-mode conversions complete synchronously and return mode bits to sleep.

## Unsupported behavior

- No full analog front-end, compensation math, timing scheduler, IIR filtering, shadow-transfer timing, or alternate SPI transport.
- No interrupt behavior because the supplied spec lists no interrupts.
- GPIO pins are documented in `sensor-model.json`; no separate GPIO CoreModel fabric is implemented for this I2C sensor instance.

## Test coverage

`tests/test_bme280.c` covers:
- Reset values and identity register.
- Read/write access policies.
- Field masks and reserved-bit preservation.
- Measurement setter encoding and XLSB reserved nibbles.
- Calibration configuration and reset side effects.
- Soft reset command side effect.
- I2C register framing and auto-increment burst reads.
- Invalid register accesses.
- Absence of interrupts.

## Build-rule compliance

- Generated files compile with strict warnings-as-errors.
- CoreModel `coremodel.c` compiles as a separate object with lenient `-Wall` and is linked into `bme280-coremodel`.
- Makefile and CMake use equivalent separation between generated code and CoreModel library code.
