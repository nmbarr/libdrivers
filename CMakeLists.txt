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
    src/bus.c
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

# ---------------------------------------------------------------------------
# Tests
#
# Host-side unit tests for the core, run with CTest:
#
#   cmake -S . -B build && cmake --build build && ctest --test-dir build
#
# Default ON when libdrivers is the project being built, OFF when it has been
# pulled into a firmware build with add_subdirectory(): a consumer wants the
# library, not its test binaries, and the tests would not link against a
# bare-metal toolchain anyway.
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
    set(LIBDRIVERS_BUILD_TESTS_DEFAULT ON)
else()
    set(LIBDRIVERS_BUILD_TESTS_DEFAULT OFF)
endif()

option(LIBDRIVERS_BUILD_TESTS "Build the host-side unit tests"
       ${LIBDRIVERS_BUILD_TESTS_DEFAULT})

# Off by default so a warning does not block someone's local build; CI turns it
# on, which is where keeping the tree warning-free is actually enforced.
option(LIBDRIVERS_WARNINGS_AS_ERRORS "Treat compiler warnings in the tests as errors" OFF)

if(LIBDRIVERS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
