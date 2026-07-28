# libdrivers

C library for embedded sensor drivers, targeting STM32L4 (I2C via the STM32 HAL).

## Current Drivers

- **LIS3MDL** — 3-axis magnetometer. Init, register read/write, WHO_AM_I check, raw axis read, hard-iron offset read.
- **HTS221** — temperature and humidity sensor. Init, register read/write, WHO_AM_I check, calibration read, raw output read, calibrated temperature and humidity.

## Layout

- `include/libdrivers/` — public headers
- `src/` — driver sources
- `datasheets/` — component datasheets for reference

## Usage

Include the relevant header and link against the library:

```c
#include "libdrivers/hts221.h"
```

## Building

Standard C11. Compile the sources in `src/` and link as needed. Requires the STM32L4xx HAL and CMSIS headers on the include path (see `compile_flags.txt`).
