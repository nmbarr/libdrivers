#ifndef LIBDRIVERS_FAKE_BUS_H
#define LIBDRIVERS_FAKE_BUS_H

#include "libdrivers/bus.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @file fake_bus.h
 * @brief In-memory Libdrivers_Bus_t for host-side driver tests.
 *
 * Stands in for a port. Backed by a 256-byte register file the test seeds with
 * whatever the device would hold, and it records every transaction so a test
 * can assert on what the driver actually put on the wire -- the register
 * address (auto-increment bit included), the direction, the length, and the
 * bytes.
 *
 * The recording matters as much as the data: several of these drivers differ
 * only in whether they set the auto-increment bit, and that is invisible in the
 * decoded sample values.
 */

/** @brief Register-file size. Covers the whole 8-bit register address space. */
#define FAKE_BUS_REG_COUNT 256

/** @brief Most transactions a fake bus records before it stops growing the log. */
#define FAKE_BUS_MAX_TRANSACTIONS 32

/** @brief Longest single transaction the log stores payload bytes for. */
#define FAKE_BUS_MAX_PAYLOAD 16

/** @brief Whether a recorded transaction was a read or a write. */
typedef enum {
    FAKE_BUS_READ,
    FAKE_BUS_WRITE,
} FakeBusOp;

/** @brief One recorded bus transaction. */
typedef struct {
    FakeBusOp op;                       /**< Read or write. */
    uint8_t reg;                        /**< Register address exactly as the driver passed it,
                                             auto-increment bit and all. */
    uint16_t len;                       /**< Byte count the driver asked for. */
    uint8_t data[FAKE_BUS_MAX_PAYLOAD]; /**< Bytes written, or bytes returned;
                                             truncated at FAKE_BUS_MAX_PAYLOAD. */
} FakeBusTransaction;

/**
 * @brief Fake bus state. Zero-initialize, then wire it with fake_bus_init().
 */
typedef struct {
    uint8_t regs[FAKE_BUS_REG_COUNT]; /**< The device's register file. Seed it directly. */

    FakeBusTransaction log[FAKE_BUS_MAX_TRANSACTIONS]; /**< Recorded transactions. */
    size_t transaction_count; /**< Transactions attempted, including any the log
                                   dropped for space and any that failed. */

    uint32_t delay_calls;    /**< Times the delay hook was called. */
    uint32_t delay_total_ms; /**< Sum of every delay duration requested. */

    // Failure injection. fail_on_call is 1-based over reads and writes
    // together, matching how a test counts the driver's steps; 0 disables.
    size_t fail_on_call;             /**< Which transaction to fail. */
    Libdrivers_Status_t fail_status; /**< Status that transaction returns. */
} FakeBus;

/**
 * @brief Point @p bus at @p fake, clearing the fake's log and register file.
 *
 * @param fake     Fake to reset and bind.
 * @param[out] bus Transport the driver handle will use.
 */
void fake_bus_init(FakeBus *fake, Libdrivers_Bus_t *bus);

/**
 * @brief Wire @p bus to @p fake but leave its delay hook NULL.
 *
 * A port is allowed to supply no delay, and a driver that needs one must say
 * so rather than silently skipping the wait (see ICM42688_Init).
 */
void fake_bus_init_without_delay(FakeBus *fake, Libdrivers_Bus_t *bus);

/**
 * @brief Make transaction number @p call_index fail with @p status.
 *
 * @param fake       Fake to arm.
 * @param call_index 1-based index over reads and writes together.
 * @param status     Status the failing transaction returns.
 */
void fake_bus_fail_on_call(FakeBus *fake, size_t call_index, Libdrivers_Status_t status);

/**
 * @brief Seed @p len consecutive registers starting at @p reg.
 *
 * @param fake Fake whose register file to write.
 * @param reg  First register address (no auto-increment bit).
 * @param data Bytes to store.
 * @param len  Number of bytes.
 */
void fake_bus_seed(FakeBus *fake, uint8_t reg, const uint8_t *data, size_t len);

/**
 * @brief Fetch a recorded transaction, or NULL if @p index is past the end.
 *
 * @param fake  Fake to inspect.
 * @param index 0-based transaction index.
 */
const FakeBusTransaction *fake_bus_transaction(const FakeBus *fake, size_t index);

#endif // LIBDRIVERS_FAKE_BUS_H
