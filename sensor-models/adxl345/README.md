# ADXL345 CoreModel Sensor Model

Generated CoreModel binding for the ADXL345 3-axis accelerometer, driven over SPI.

## Interface

- Bus: SPI, chip select 0.
- Command framing: bit 7 selects read vs. write, bit 6 selects multi-byte transfers, bits 5:0 are the register address (`ADXL345_SPI_READ`, `ADXL345_SPI_MULTIBYTE`, `ADXL345_SPI_ADDR_MASK` in `adxl345_model.h`).
- `DEVID` (0x00, ro, reset `0xE5`) is the identity register.
- `POWER_CTL` (0x2D, rw) exposes the `MEASURE` bit (bit 3); all other bits are reserved and stay zero.
- `DATAX0`/`DATAX1`, `DATAY0`/`DATAY1`, `DATAZ0`/`DATAZ1` (0x32-0x37, ro) expose little-endian signed acceleration samples.

## Integration

- `adxl345_reset()` initializes register state, including the `DEVID` reset value.
- `adxl345_reg_read()` / `adxl345_reg_write()` implement the register-level access policy (read-only, write-only, reserved-bit masking).
- `adxl345_set_acceleration(dev, x, y, z)` is the injection point: call it from a test harness or the CoreModel SPI adapter to drive simulated motion into `DATAX0..DATAZ1`. This is the sensor-pattern equivalent of an actuator's output getter — the host supplies the physical input, the model exposes it on the bus.
- `adxl345_spi_cs()` / `adxl345_spi_xfr()` bind the register model to CoreModel's SPI callback shape (`coremodel_spi_func_t`, see `adxl345_coremodel.c`).

## Evidence used

- `coremodel.h` / CoreModel `README.md`: SPI callback signature, `coremodel_attach_spi`, `COREMODEL_SPI_BLOCK`, `coremodel_mainloop`, detach/disconnect semantics.
- `examples/spi-ds3234-rtc/spi-ds3234.c`: nearest SPI register-backed model pattern (command byte decode, multi-byte transfer handling).
- `examples/i2c-adxl36x/adxl36x_registers.c`: referenced only for Analog Devices accelerometer register naming conventions, not for behavior.
- `examples/list/coremodel-list.c`: enumeration pattern referenced by integration docs.

## Assumptions

- ADXL345-style SPI command framing: bit 7 read, bit 6 multi-byte, bits 5:0 address.
- Invalid reads return `0x00`; invalid writes are ignored.
- Only the documented `POWER_CTL.MEASURE` bit is writable; reserved bits remain zero.
- Samples are injected through `adxl345_set_acceleration()`; no physical motion model is invented.

## Unsupported behavior

FIFO, activity/inactivity detection, data-ready interrupts/status, self-test, scaling, offsets, timing, and physical motion are unsupported because the normalized spec (`sensor-model.json`) did not include them. The model does not implement an SPI controller; it binds to CoreModel's SPI callbacks.

## Tests

`tests/test_adxl345.c` covers reset values, identity readback, read/write access policy, field masking of reserved bits, sample injection via `adxl345_set_acceleration()`, SPI single/multi-byte framing, absence of read side effects, empty interrupt behavior, and invalid accesses.

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

See `GENERATION_REPORT.md` for the full generation record.
