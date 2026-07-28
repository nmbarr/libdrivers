# Making libdrivers Vendor-Agnostic

A design discussion on removing the hardcoded STM32 HAL dependency from the
sensor drivers and replacing it with a portable bus interface.

## Problem: where the HAL is baked in

There are three distinct kinds of coupling to the STM32 HAL, each with a
different fix:

1. **The transport calls (the important one).** `ReadReg`/`WriteReg` in
   `lis3mdl.c` call `HAL_I2C_Mem_Read` / `HAL_I2C_Mem_Write` directly. This is
   the actual vendor lock — every byte in and out of the chip goes through
   STM32's I2C driver.

2. **The handle holds an STM32 type.** `LIS3MDL_Handle_t` contains an
   `I2C_HandleTypeDef *hi2c`, so even the shape of the handle is
   STM32-specific.

3. **The return type is `HAL_StatusTypeDef`.** Every public function returns
   STM32's status enum, and the headers `#include "stm32l4xx_hal.h"`. This is
   the most viral coupling: anyone consuming the library — even on a non-STM32
   chip — is forced to pull in the entire STM32 HAL just to name the type.

What is already portable: the register maps, the bit-shifting in
`ReadRaw`/`ReadHardIronOffset`, and the WHO_AM_I logic. The chip *knowledge* is
clean. Only the plumbing is tied down.

## Approach: a bus interface struct

Invert the dependency. The driver calls *through* function pointers instead of
calling the HAL by name. The handle holds those pointers plus an opaque
`void *ctx`. The driver never knows what `ctx` is — on STM32 you stuff your
`I2C_HandleTypeDef*` in there; on another chip, something else; in a unit test,
a fake.

This single change dissolves all three couplings at once: the transport becomes
a call through a pointer, the handle no longer names an STM32 type, and the HAL
include can be dropped from the header. It is the same pattern ST's own
platform-independent driver templates and Bosch's BME280 driver use.

## Where the STM32 glue lives

Options considered:

- **A — entirely in the application.** Library ships only the portable core;
  the app writes the ~15-line adapter. Library has zero STM32 dependency, but
  every consuming project rewrites boilerplate.
- **B — optional port directory inside the library (`port/stm32/`).** Portable
  core plus a ready-made, opt-in STM32 adapter as a separate translation unit.
- **C — status quo, glue inline in each driver.** Rejected; it's what we're
  removing.

**Decision: B.** Since we develop on real STM32L4 hardware, shipping the port
means we dogfood the exact interface an external user gets — the best way to
find out if the interface is any good. If the adapter turns out perfectly
generic we keep it; if it's project-specific we delete it and fall back to A.

Proposed structure:

```
include/libdrivers/
    bus.h            <- interface typedefs + status enum (no HAL)
    lis3mdl.h        <- pure, includes bus.h only
src/
    lis3mdl.c        <- pure, calls through function pointers
port/stm32/
    libdrivers_stm32_i2c.h/.c   <- the ONLY files that #include stm32*.h
```

The rule that keeps it honest: **the STM32 header may appear only under
`port/`.** If it creeps back into `src/` or `include/libdrivers/`, portability
is broken and it's grep-able in one command (enforceable in CI later). The port
is decoupled at the build level too — the core builds and passes tests with no
STM32 headers present; the port is a separate target. That's what proves the
decoupling is real rather than cosmetic.

## The bus.h contract

`bus.h` is the one file both the portable core and every port agree on. Its job
is to be small, stable, and impossible to accidentally couple to a vendor.

### Transport functions

- **read:**  `(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len)`
- **write:** `(void *ctx, uint8_t reg, const uint8_t *buf, uint16_t len)`

Notes baked into this shape:

- `ctx` is opaque and comes first (conventional C callback idiom). The core
  never dereferences it.
- `reg` is just the register address, **without** the auto-increment bit.
  Auto-increment stays in the driver — it's chip knowledge (LIS3MDL uses bit 7;
  other chips differ). The bus stays dumb: "write these bytes to this
  register." That keeps the interface identical across all three sensors.
- `len` is `uint16_t` — covers any FIFO burst read.

### Device address — in ctx

**Decision: address lives in `ctx`, the port owns it.** The bus signatures have
no address argument at all. The port is handed the address at setup and bakes
it into its context.

Consequences:

- The driver's mental model is purely "put these bytes at this register on my
  device." It has no concept of *which* device. This is also what makes
  SPI-vs-I2C irrelevant to the core later: an SPI port encodes chip-select in
  `ctx` the same way an I2C port encodes the slave address.
- `#define LIS3MDL_I2C_ADDR 0x3C` leaves the driver entirely — it becomes a
  wiring detail the app supplies. The SA0-strapped alternate address is just
  "put a different number in the ctx," zero driver changes.
- The `I2C_HandleTypeDef *hi2c` field leaves `LIS3MDL_Handle_t`.

The one thing to be deliberate about: **ctx lifetime and ownership.** The
context struct (handle + address) must outlive the sensor handle, because the
driver holds a pointer into it and calls back through it on every read. On
bare-metal STM32 this is a non-issue (declare it static/global), but it bites
people who allocate on a stack frame and return. Convention: **app owns the ctx
struct, static lifetime**, with a one-line comment in the port saying so.

### Status type

Replace `HAL_StatusTypeDef` with our own small enum: `OK`, `ERR_BUS` (transport
failed), `ERR_ARG` (bad argument / missing capability), `ERR_ID` (WHO_AM_I
mismatch), and optionally `ERR_TIMEOUT`.

The port's job is to collapse `HAL_StatusTypeDef` down to this enum
(`HAL_OK → OK`, else `ERR_BUS`, optionally `HAL_TIMEOUT → ERR_TIMEOUT`). The
driver only ever speaks our enum — that's what lets the headers drop the STM32
include.

This also unlocks fixing `CheckWhoAmI`, which currently returns `bool` and so
conflates "I2C failed" with "chip present but wrong ID." With a real status
enum it can fold into the normal return convention and stop losing that
distinction.

### Delay hook

**Decision: include it in the struct now.** Signature
`(void *ctx, uint32_t ms)` — takes `ctx` for symmetry even though it usually
ignores it, and **may be NULL**.

LIS3MDL init doesn't need it, but LSM6DSL wants a wait after `BOOT`/`SW_RESET`
and HTS221 has boot timing. Designing it in now means adding reset logic later
is a no-op to the contract instead of a breaking rev of every port.

NULL convention (must be consistent across all drivers or it's a trap): a
driver that reaches a point where it genuinely needs to delay and finds
`delay_ms == NULL` **returns an error** (`ERR_ARG` — the port didn't supply a
required capability). It never silently skips the wait, because a skipped
post-reset delay is a heisenbug. Drivers that never delay simply never touch
the pointer. State this convention in a comment in `bus.h`.

### Struct and handle

- **One struct** grouping the three pointers + the `void *ctx`, defined once in
  `bus.h`, included by all three sensors — not a per-sensor copy. This is what
  makes them a *library* rather than three unrelated files.
- The **handle** holds that struct **by value** to start (self-contained, no
  lifetime worry about a separately-allocated interface; the few extra bytes
  don't matter on this target). A pointer-to-shared-bus variant is possible if
  multiple sensors share one physical bus, but by-value is the simpler start.
- **Addresses are runtime data** in the port's ctx, never `#define`s in the
  driver. `bus.h` stays free of any address define.

## Settled contract summary

- Status enum: OK + bus/arg/id (+ optional timeout).
- Read pointer: `(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len)`, no
  address arg.
- Write pointer: same with `const uint8_t *buf`.
- Delay pointer: `(void *ctx, uint32_t ms)`, may be NULL, error cleanly if
  needed-but-missing.
- One struct groups the three pointers + ctx; defined once, shared by all
  drivers.
- Handle holds that struct by value instead of an `I2C_HandleTypeDef*`.
- Addresses supplied by the app through ctx.
- No STM32 anything in `bus.h`.

## Migration order

- **LIS3MDL first** — the most complete driver and the only one with real logic
  (ReadRaw, ReadHardIronOffset, WhoAmI, the CTRL-reg init loop). Converting it
  proves the interface against real usage and flushes out signature mistakes
  before they're copied into the others. Convert it end-to-end (driver core +
  the STM32 port under `port/stm32/`), get it compiling and ideally reading the
  chip, then use it as the mold for the other two.
- **LSM6DSL is a stub**, so it's not a test of anything yet — but whatever
  pattern LIS3MDL establishes becomes the template it's built onto.
- **HTS221** has the delay-hook angle (boot timing), so it's the target that
  actually exercises the NULL-delay convention — a good second conversion to
  validate that part of the contract.

The order means interface problems surface while only one driver depends on the
interface, not three.
