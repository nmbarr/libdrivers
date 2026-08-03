cmake_minimum_required(VERSION 3.15)

project(libdrivers
    VERSION 0.1.0
    DESCRIPTION "Vendor-agnostic embedded sensor drivers"
    LANGUAGES C
)

# ---------------------------------------------------------------------------
# Core driver library
#
# This target is intentionally HAL-free: it compiles against nothing but the
# C standard library and its own headers under include/. It builds on any
# host with a C compiler, which is what makes the drivers unit-testable off
# target. Vendor specifics live in port/ and are NOT built here (see below).
# ---------------------------------------------------------------------------
# Target is named "drivers" (not "libdrivers"): CMake prepends the "lib"
# prefix itself, so this produces libdrivers.a and links as -ldrivers.
add_library(drivers
    src/ds18b20.c
    src/hts221.c
    src/icm42688.c
    src/lis3mdl.c
    src/lps22hb.c
    src/lsm6dsl.c
)

# include/ is PUBLIC: the driver sources need it to find their own headers,
# and any consumer that links the library inherits it automatically, so they
# can #include "libdrivers/<driver>.h" without setting their own paths.
target_include_directories(drivers PUBLIC include)

# The drivers are written to C11 (designated initializers, <stdint.h>, etc.).
# PUBLIC so consumers compile the headers under the same standard.
target_compile_features(drivers PUBLIC c_std_11)

# ---------------------------------------------------------------------------
# Ports (STM32, etc.) are deliberately NOT built by this CMakeLists.
#
# A port (e.g. port/stm32/libdrivers_stm32_i2c.c) includes the vendor HAL
# (stm32l4xx_hal.h), which does not live in this repo -- it comes from the
# firmware project / CubeMX. Building it here would mean hardcoding paths to
# a toolchain this library does not own.
#
# Instead, add the port source to your firmware project's build, alongside
# this library. The firmware project already has the HAL on its include path.
# This keeps the core genuinely vendor-neutral: this file never names a HAL.
# ---------------------------------------------------------------------------
