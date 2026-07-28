# libdrivers

C library for embedded sensor drivers.

## Current Drivers

- **LIS3MDL** - 3-axis magnetometer
- **HTS221** - Temperature and humidity sensor

## Datasheets

Component datasheets are stored in `datasheets/` for reference.

## Usage

Include the relevant header and link against the library:

```c
#include "libdrivers/lis3mdl.h"
```

## Building

Standard C library build. Compile source files in `src/` and link as needed.