# target-tools/example-spi-dac-actuator

Two separate pieces, on two separate machines:

```text
  host/    -> builds & runs on YOUR MAC (the sensor/model side)
  target/  -> cross-compiled & runs ON THE IMX93 (the SPI master side)
```

```text
  [ host/run-example-spi-dac-actuator-verbose ]  <--- network: 10.11.1.11:1900 --->  [ CoreModel VM ]
          (your Mac — the DAC model)                                                          |
                                                                                   virtual lpspi0 bus
                                                                                                |
                                                                         [ imx93evk Linux, /dev/spidev0.0 ]
                                                                         [ target/spi_dac_tool ]
                                                                         (the imx93 — the SPI master)
```

Never build or copy `host/` onto the imx93, and never try to cross-compile
or run `target/spi_dac_tool` on your Mac.

## `host/` — runs on your Mac

`run-example-spi-dac-actuator-verbose` is the sensor side: it attaches to
CoreModel exactly like `sensor-models/example-spi-dac-actuator/run-example-spi-dac-actuator`
does (same `coremodel_connect` / `coremodel_attach_spi` calls, same register
model), but traces every CS change and byte transferred to stdout — the
plain `run-example-spi-dac-actuator` is silent by design.

`example_spi_dac_actuator.{c,h}` and `coremodel.{c,h}` in this folder are
vendored copies of `sensor-models/example-spi-dac-actuator/{src,include}/`
and `coremodel/`, so `host/` builds standalone. This is hand-written
debug/demo tooling, not output of the sensor-model-builder agent — kept out
of `sensor-models/example-spi-dac-actuator/` since that directory is
reserved for the agent's generated files. If the agent regenerates the
model, re-copy `example_spi_dac_actuator.{c,h}` here to stay in sync.

Build and run, on your Mac:

```sh
cd host
make
./run-example-spi-dac-actuator-verbose 10.11.1.11:1900 lpspi0 0
```

Stop any other `run-example-spi-dac-actuator*` already attached to the same
bus/CS first — only one process can hold a given chip-select at a time.

Live trace output looks like:

```text
ExampleSpiDacActuator (verbose) attached to SPI lpspi0 CS0.
Protocol: command byte bit7=read, bits6:0=register; 16-bit data is little-endian.
CS0 asserted
TX [1] 8f
RX [1] 00
TX [1] 00
RX [1] c1
TX [1] 00
RX [1] 5a
```

(`8f` = read DEVICE_ID; response bytes `c1 5a` little-endian = `0x5ac1`.)

## `target/` — cross-compiled for, and runs on, the imx93

`spi_dac_tool` is the SPI master: it opens `/dev/spidevB.C` and issues the
`ExampleSpiDacActuator` register protocol — a 3-byte, single-CS-assertion
transaction (command byte: `bit7`=read, `bits6:0`=register; then a 16-bit
little-endian data field). It sends this as three chained 1-byte, 8-bit-word
transfers inside one `SPI_IOC_MESSAGE(3)` ioctl call — Linux only releases CS
after the last transfer in the message, so all 3 bytes land in one CS
assertion without depending on a non-standard `bits_per_word` (24-bit words
returned `EINVAL` on at least one i.MX93 LPSPI driver).

Cross-compile from your host and copy the binary to the imx93:

```sh
cd target
make PREFIX=aarch64-linux-gnu-
scp spi_dac_tool root@<imx93>:~/
```

Or build directly on the target if it has `gcc` and kernel headers:

```sh
cd target
make
```

### Finding the right /dev/spidev path, and matching the chip-select

CoreModel's `lpspi0` is 0-indexed by *silicon* LPSPI controller number, but
Linux's `/dev/spidevB.C` is indexed by the order controllers are enabled in
the DTB. They are not guaranteed to match — confirm with:

```sh
ls -l /sys/bus/spi/devices/
```

**The chip-select argument to `run-example-spi-dac-actuator-verbose` must
match the `spidev` child's `reg` value in the devicetree.** If your DTB only
defines a `spidev` child at `reg = <0>` (chip-select 0) under the controller
that maps to `lpspi0`, attach with `... lpspi0 0` and use `/dev/spidevB.0`.
If nothing shows up under `/dev/spidev*` at all, the SPI controller itself is
disabled in the DTB — see `coremodel/examples/spi-ds3234-rtc/README.md` for
the full DTB-enablement walkthrough (uploading a patched devicetree via the
VM's Settings → Device Tree page).

### Usage, on the imx93

Replace `/dev/spidevB.C` with whatever bus/CS actually matches:

```sh
./spi_dac_tool /dev/spidevB.C id
./spi_dac_tool /dev/spidevB.C ctrl
./spi_dac_tool /dev/spidevB.C code 2048
./spi_dac_tool /dev/spidevB.C ctrl 1 0
./spi_dac_tool /dev/spidevB.C code
./spi_dac_tool /dev/spidevB.C status
./spi_dac_tool /dev/spidevB.C read 0x0f
./spi_dac_tool /dev/spidevB.C write 0x01 4095
```

### If a transfer fails with "Invalid argument" (EINVAL)

Run the built-in diagnostic — it probes the spidev node with several
increasingly specific ioctl calls (read back defaults, a 1-byte transfer, a
single 3-byte transfer, 3 chained 1-byte transfers, then the same after
forcing mode/bits/speed) and reports exactly which step fails and the errno
for it:

```sh
./spi_dac_tool /dev/spidevB.C diag
```

### Alternative: plain `spidev_test`

`spidev_test` (present on most i.MX93 Yocto images) can also drive this
protocol, but only if its `-b 24` word-size trick is accepted by your
kernel's LPSPI driver — it returned `EINVAL` in testing on at least one
i.MX93 image, which is why `spi_dac_tool` uses chained 8-bit transfers
instead. Worth trying first since it needs no build step:

```sh
# read DEVICE_ID (reg 0x0f -> cmd byte 0x8f)
spidev_test -D /dev/spidevB.C -b 24 -p "\x8f\x00\x00"

# write DAC_CODE = 2048 (0x800) -> cmd 0x01, data 00 08
spidev_test -D /dev/spidevB.C -b 24 -p "\x01\x00\x08"
```

If that fails with "Invalid argument", use `spi_dac_tool` instead.
