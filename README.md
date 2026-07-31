# libdrivers

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
- `port/stm32/libdrivers_stm32_onewire.{c,h}` — bit-banged GPIO 1-Wire using a
  DWT cycle-counter µs timer. Wire it with `Libdrivers_STM32_OneWire_InitBus`.

## Layout

- `include/libdrivers/` — public headers (Doxygen-documented)
- `src/` — HAL-free driver + contract sources
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

A working consumer is the B-L4S5I-IOT01A firmware, which pins this repo as a
submodule and builds a chosen port into its CubeMX project.
