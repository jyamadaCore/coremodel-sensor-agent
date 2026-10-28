# ADXL345 Generation Report

## Validation
`validateSensorSpecTool` returned `valid=true`. SPI `address` was normalized to `0`; `chipSelect` is `0`. The validator warning about identity registers is noted; the generated model treats supplied `DEVID` at `0x00` as identity.

## CoreModel evidence files
- `README.md`: SPI callback, attach, mainloop, list semantics.
- `coremodel.h`: authoritative declarations for `coremodel_spi_func_t`, `coremodel_attach_spi`, `COREMODEL_SPI_BLOCK`, `coremodel_mainloop`, detach/disconnect.
- `examples/spi-ds3234-rtc/spi-ds3234.c`: nearest SPI register-backed model pattern.
- `examples/i2c-adxl36x/adxl36x_registers.c`: nearby Analog Devices accelerometer register naming; not used to infer extra behavior.
- `examples/list/coremodel-list.c`: enumeration pattern referenced by integration docs.

## Generated files
`adxl345_model.h`, `adxl345_model.c`, `adxl345_coremodel.c`, `tests/test_adxl345.c`, `CMakeLists.txt`, `Makefile`, `README.md`, `sensor-model.json`, `GENERATION_REPORT.md`.

## Assumptions
- ADXL345-style SPI command framing: bit7 read, bit6 multi-byte, bits5:0 address.
- Invalid reads return `0x00`; invalid writes are ignored.
- Only documented `POWER_CTL.MEASURE` bit 3 is writable; reserved bits remain zero.
- Samples are injected through `adxl345_set_acceleration()`; no physical model is invented.

## Unsupported behavior
FIFO, activity/inactivity detection, data-ready interrupts/status, self-test, scaling, offsets, timing, and physical motion are unsupported because the normalized spec did not include them. The model does not implement an SPI controller; it binds to CoreModel SPI callbacks.

## Test coverage
Tests cover reset values, identity readback, read/write policy, field masking/reserved bits, sample injection, SPI single/multi-byte framing, no read side effects, empty interrupt behavior, and invalid accesses.

## Post-generation correction
The plain Makefile's `app` target (and the equivalent CMakeLists.txt target, previously gated behind a global `CMAKE_C_FLAGS`) applied this model's `-Wall -Wextra -Werror` policy to the vendored `coremodel.c` when linking the CoreModel-bound executable, which fails to compile on toolchains that flag `coremodel.c`'s sign-compare usage as an error. Both build files were hand-patched to compile `coremodel.c` as a separate object/source with lenient flags (matching the CoreModel library's own Makefile), leaving this model's own strict flags untouched. Re-verified: `make test`, `make app COREMODEL_DIR=../../coremodel`, and the equivalent `cmake`/`ctest` path all pass, including the real CoreModel-linked `adxl345-coremodel` executable.
