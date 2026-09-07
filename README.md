# libdrivers

[![CI](https://github.com/nmbarr/libdrivers/actions/workflows/ci.yml/badge.svg)](https://github.com/nmbarr/libdrivers/actions/workflows/ci.yml)

Vendor-agnostic C library of embedded sensor drivers. The driver core is
**HAL-free**: drivers never include a vendor HAL. Instead they talk to hardware
through small function-pointer transport contracts, and a thin per-platform
*port* implements those hooks. Swapping MCUs (or unit-testing on a host) means
writing a new port, not touching a driver.

## Architecture

Drivers depend on one of two transport contracts (see `include/libdrivers/`):

- **`Libdrivers_Bus_t`** (`bus.h`) — register bus for I2C/SPI parts. Three hooks
  (`read` / `write` / `delay`) plus an opaque `ctx`. Used by the register-mapped
  sensors below.
- **`Libdrivers_OneWire_t`** (`onewire.h`) — byte-level 1-Wire transport
  (`reset` / `write` / `read` + `ctx`). The port hides all µs bit-timing.

Both share the `Libdrivers_Status_t` return enum, so a driver returns one status
type regardless of transport. `src/bus.c` holds shared helpers such as
`Libdrivers_Bus_CheckWhoAmI`.

## Drivers

| Driver   | Part                          | Transport            |
|----------|-------------------------------|----------------------|
| HTS221   | Humidity + temperature        | `Libdrivers_Bus_t`   |
| LIS3MDL  | 3-axis magnetometer           | `Libdrivers_Bus_t`   |
| LSM6DSL  | 3-axis accelerometer + gyro   | `Libdrivers_Bus_t`   |
| LPS22HB  | Pressure + temperature        | `Libdrivers_Bus_t`   |
| ICM42688 | 3-axis accelerometer + gyro   | `Libdrivers_Bus_t`   |
| DS18B20  | 1-Wire temperature            | `Libdrivers_OneWire_t` |

Each register-bus driver provides Init, register read/write, a WHO_AM_I check,
and raw/calibrated sample reads. DS18B20 uses a presence-check Init plus a
two-step `StartConversion` / `ReadTemperature` protocol (single-device, Skip ROM).

## Ports

Ports live in `port/` and are the **only** place a vendor HAL is included. They
are *not* built by this library's CMake — the HAL belongs to the consuming
firmware/CubeMX project, which grafts the port source into its own build.

- `port/stm32/libdrivers_stm32_i2c.{c,h}` — wraps `HAL_I2C_Mem_Read/Write` +
  `HAL_Delay` for `Libdrivers_Bus_t`. Wire it with `Libdrivers_STM32_I2C_InitBus`.
- `port/stm32/libdrivers_stm32_spi.{c,h}` — wraps `HAL_SPI_Transmit/Receive` +
  `HAL_Delay` for `Libdrivers_Bus_t`, applying the SPI R/W address bit and
  driving a manual chip-select GPIO. Wire it with `Libdrivers_STM32_SPI_InitBus`.
- `port/stm32/libdrivers_stm32_onewire.{c,h}` — bit-banged GPIO 1-Wire using a
  DWT cycle-counter µs timer. Wire it with `Libdrivers_STM32_OneWire_InitBus`.
- `port/stm32/libdrivers_stm32_common.{c,h}` — shared HAL-status translation
  used by the I2C and SPI ports. Add it to the firmware build alongside either.

## Layout

- `include/libdrivers/` — public headers (Doxygen-documented)
- `src/` — HAL-free driver + contract sources
- `test/` — host-side unit tests and the transport fakes they run against
- `port/stm32/` — STM32 HAL adapters (built by the firmware, not here)
- `datasheets/` — component datasheets for reference

## Building

Builds the HAL-free core as a static library (`libdrivers.a`) — no vendor HAL
required:

```sh
cmake -S . -B build && cmake --build build
```

`include/` is exported `PUBLIC`; the standard is C11. Host toolchains (e.g. plain
gcc) work, since the core pulls in no MCU headers.

## Tests

The drivers reach hardware only through the function-pointer transports in
`bus.h` / `onewire.h`, which is what makes them testable off target: a test
supplies its own implementation of a transport and runs the real driver code on
the host. No hardware, no HAL, no target toolchain.

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

`test/` holds two fakes and one suite per driver:

- **`fake_bus`** — a `Libdrivers_Bus_t` over a 256-byte register file. It records
  every transaction, so a test can assert what the driver actually put on the
  wire: the register address *including the auto-increment bit*, the direction,
  the length and the bytes. That recording matters as much as the decoded value,
  because several of these drivers differ only in whether they set that bit, and
  the difference is invisible in the sample they return.
- **`fake_onewire`** — a `Libdrivers_OneWire_t` that replays a scripted byte
  sequence and logs the command bytes, so the DS18B20's reset / Skip ROM /
  function-command ordering can be checked.

Both fakes inject a transport failure at a chosen step, which is how the
"stops at the first failing write" and "leaves the output untouched on failure"
cases are driven.

What the suites pin down, beyond the happy path:

- **Register traffic** — the exact registers `Init` writes, their order (the
  ICM42688 must configure full-scale *before* PWR_MGMT0 powers the sensors on),
  and that a failing write stops the sequence instead of half-configuring a part.
- **The auto-increment bit** — set by HTS221/LIS3MDL/LPS22HB, and deliberately
  *not* set by LSM6DSL/ICM42688, which auto-increment internally.
- **Byte order and sign** — little-endian for the ST parts, big-endian for the
  ICM42688, across the full `int16_t` range; the LPS22HB's 24-bit pressure at
  all four of its extremes.
- **Scaling** — every full-scale code of both IMUs against its datasheet
  sensitivity, including the LSM6DSL's `FS_125` bit overriding `FS_G`, and the
  DS18B20's datasheet temperature/data table.
- **Failure contracts** — a transport error propagates unchanged rather than
  being flattened into `ERR_ID`, and a failed read leaves the caller's output
  variable alone.

The suite is mutation-tested: deliberately breaking a driver (dropping the
auto-increment bit, swapping a byte order, checking `FS_G` before `FS_125`,
accepting a mismatched WHO_AM_I) must make it fail. A test that cannot fail is
not a test.

## CI

`.github/workflows/ci.yml` runs on every push to `main` and every pull request.
Because `port/` needs a vendor HAL that lives in the consuming firmware project,
CI builds only the HAL-free core; the ports are still format-checked.

| Job           | What it checks                                                       |
|---------------|----------------------------------------------------------------------|
| `build`       | The core and tests compile under gcc *and* clang with `-Wall -Wextra -Wpedantic -Werror`, the suite passes, the library links with no undefined symbols, and every public header compiles standalone |
| `cross-build` | The core also builds bare-metal for Cortex-M4 with `arm-none-eabi-gcc`, and reports per-driver flash cost |
| `sanitizers`  | The suite passes under AddressSanitizer + UndefinedBehaviorSanitizer  |
| `format`      | `src/`, `include/`, `port/` and `test/` match `.clang-format`          |
| `tidy`        | `clang-tidy` finds nothing under the checks in `.clang-tidy`          |

The undefined-symbol check builds the same sources as a shared object with
`-Wl,--no-undefined`. A static archive links fine with unresolved symbols in it,
so without this a source file missing from `CMakeLists.txt` would only surface
at firmware link time, in someone else's project.

The sanitizer job earns its place: the tests push the drivers through boundary
values and injected failures, which is exactly the traffic worth running under
UBSan. It found a real undefined shift in the LPS22HB pressure decode.

Reproduce any of it locally (clang 18 is what CI pins):

```sh
# build job
cmake -S . -B build -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror" \
      -DLIBDRIVERS_WARNINGS_AS_ERRORS=ON
cmake --build build && ctest --test-dir build --output-on-failure

# sanitizers job
cmake -S . -B build-san \
      -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-san && ctest --test-dir build-san --output-on-failure

# format job
clang-format --dry-run -Werror src/*.c include/libdrivers/*.h port/stm32/*.{c,h} \
      test/*.{c,h}

# tidy job
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build src/*.c test/*.c
```

Tests are built when libdrivers is the top-level project and skipped when it is
pulled into a firmware build with `add_subdirectory()`; set
`-DLIBDRIVERS_BUILD_TESTS=OFF` to skip them explicitly.

## Usage

Include the driver header, give its handle a bus/1-wire transport via a port,
then call the driver:

```c
#include "libdrivers/hts221.h"
#include "libdrivers_stm32_i2c.h"

static Libdrivers_STM32_I2C_Context_t ctx = { .hi2c = &hi2c2, .device_addr = 0x5F << 1 };
HTS221_Handle_t hts221;
Libdrivers_STM32_I2C_InitBus(&hts221.bus, &ctx);

if (HTS221_CheckWhoAmI(&hts221) != LIBDRIVERS_OK) { /* handle */ }
```

An SPI part wires the same way, but its port context also names the
chip-select GPIO, and the driver takes a `Config` at `Init`:

```c
#include "libdrivers/icm42688.h"
#include "libdrivers_stm32_spi.h"

static Libdrivers_STM32_SPI_Context_t ctx = {
    .hspi = &hspi1, .cs_port = GPIOA, .cs_pin = GPIO_PIN_4,
};
ICM42688_Handle_t icm;
Libdrivers_STM32_SPI_InitBus(&icm.bus, &ctx);

if (ICM42688_CheckWhoAmI(&icm) != LIBDRIVERS_OK) { /* handle */ }

// Power both sub-sensors and pick full-scale + ODR (see the datasheet
// for the PWR_MGMT0 / *_CONFIG0 field encodings).
ICM42688_Config_t cfg = { .PwrMgmt0 = 0x0F, .GyroConfig0 = 0x06, .AccelConfig0 = 0x06 };
ICM42688_Init(&icm, &cfg);

ICM42688_AccelData_mg_t accel;
ICM42688_ReadAccel_mg(&icm, &accel);
```

A working consumer is the [hottub_monitor](https://github.com/nmbarr/hottub_monitor)
firmware, which pins this repo as a submodule and builds a chosen port into its
CubeMX project.
