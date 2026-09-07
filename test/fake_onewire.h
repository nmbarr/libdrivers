#ifndef LIBDRIVERS_FAKE_ONEWIRE_H
#define LIBDRIVERS_FAKE_ONEWIRE_H

#include "libdrivers/onewire.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @file fake_onewire.h
 * @brief In-memory Libdrivers_OneWire_t for host-side DS18B20 tests.
 *
 * 1-Wire has no register addressing, so unlike the fake register bus there is
 * nothing to look up: the fake replays a scripted byte sequence for reads and
 * records the command bytes the driver wrote. Asserting on that write log is
 * the only way to check the driver issued the right ROM and function commands
 * in the right order.
 */

/** @brief Most bytes the fake will hand back across a test. */
#define FAKE_ONEWIRE_MAX_READS 16

/** @brief Most command bytes the fake records. */
#define FAKE_ONEWIRE_MAX_WRITES 16

/** @brief Which hook a recorded step went through. */
typedef enum {
    FAKE_ONEWIRE_RESET,
    FAKE_ONEWIRE_WRITE,
    FAKE_ONEWIRE_READ,
} FakeOneWireOp;

/**
 * @brief Fake 1-Wire state. Zero-initialize, then wire it with fake_onewire_init().
 */
typedef struct {
    uint8_t read_script[FAKE_ONEWIRE_MAX_READS]; /**< Bytes served to reads, in order. */
    size_t read_script_len;                      /**< How many of those are valid. */
    size_t reads_served;                         /**< How many have been handed out. */

    uint8_t writes[FAKE_ONEWIRE_MAX_WRITES]; /**< Command bytes the driver sent. */
    size_t write_count;                      /**< Writes attempted, dropped ones included. */

    uint32_t reset_calls; /**< Times the reset hook was called. */

    // Step ordering, so a test can assert the whole transaction shape --
    // reset, then commands, then reads -- not just the bytes in isolation.
    FakeOneWireOp steps[FAKE_ONEWIRE_MAX_READS + FAKE_ONEWIRE_MAX_WRITES + 4];
    size_t step_count;

    // Failure injection. fail_on_step is 1-based over every hook call
    // (resets, writes and reads together); 0 disables.
    size_t fail_on_step;             /**< Which step to fail. */
    Libdrivers_Status_t fail_status; /**< Status that step returns. */
} FakeOneWire;

/**
 * @brief Point @p ow at @p fake, clearing the fake's script and logs.
 */
void fake_onewire_init(FakeOneWire *fake, Libdrivers_OneWire_t *ow);

/**
 * @brief Queue @p len bytes for the driver's reads to consume in order.
 */
void fake_onewire_script_reads(FakeOneWire *fake, const uint8_t *data, size_t len);

/**
 * @brief Make step number @p step_index fail with @p status.
 *
 * @param fake       Fake to arm.
 * @param step_index 1-based index over resets, writes and reads together.
 * @param status     Status the failing step returns.
 */
void fake_onewire_fail_on_step(FakeOneWire *fake, size_t step_index, Libdrivers_Status_t status);

/**
 * @brief Fetch a recorded step kind, or a count-past-end marker.
 *
 * @return The step's op, or FAKE_ONEWIRE_RESET if @p index is out of range;
 *         callers should check step_count first.
 */
FakeOneWireOp fake_onewire_step(const FakeOneWire *fake, size_t index);

#endif // LIBDRIVERS_FAKE_ONEWIRE_H
