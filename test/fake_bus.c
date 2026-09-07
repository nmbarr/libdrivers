#include "fake_bus.h"
#include <string.h>

// The auto-increment bit is an addressing modifier, not part of the register
// address: the ST parts here OR it into the address byte to ask for a burst.
// Strip it to find which register the driver meant, and keep the raw byte in
// the log so a test can assert the driver set (or did not set) it.
#define FAKE_BUS_AUTO_INCREMENT_BIT 0x80

/**
 * @brief Record a transaction and decide whether this call should fail.
 *
 * Counts every attempt, so an injected failure lands on the call the test
 * named whether or not earlier calls were reads or writes.
 */
static Libdrivers_Status_t fake_bus_record(FakeBus *fake, FakeBusOp op, uint8_t reg,
                                           const uint8_t *data, uint16_t len) {
    fake->transaction_count++;

    if (fake->transaction_count <= FAKE_BUS_MAX_TRANSACTIONS) {
        FakeBusTransaction *entry = &fake->log[fake->transaction_count - 1];
        entry->op = op;
        entry->reg = reg;
        entry->len = len;

        size_t copy = len < FAKE_BUS_MAX_PAYLOAD ? len : FAKE_BUS_MAX_PAYLOAD;
        memcpy(entry->data, data, copy);
    }

    if (fake->fail_on_call != 0 && fake->transaction_count == fake->fail_on_call) {
        return fake->fail_status;
    }

    return LIBDRIVERS_OK;
}

/**
 * @brief Read hook: serve @p len bytes from the register file.
 *
 * Reads walk consecutive registers from the base address, which is what both
 * the auto-increment bit and the LSM6DSL/ICM42688 internal IF_INC do. The
 * address wraps at 256 so a burst off the end of the file cannot run out of
 * bounds.
 *
 * A failing read leaves @p buf untouched, matching a port that never received
 * the bytes -- so a test can prove the driver did not consume garbage.
 */
static Libdrivers_Status_t fake_bus_read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len) {
    FakeBus *fake = (FakeBus *)ctx;
    uint8_t base = reg & (uint8_t)~FAKE_BUS_AUTO_INCREMENT_BIT;

    uint8_t staged[FAKE_BUS_REG_COUNT];
    for (uint16_t i = 0; i < len && i < FAKE_BUS_REG_COUNT; i++) {
        staged[i] = fake->regs[(base + i) % FAKE_BUS_REG_COUNT];
    }

    Libdrivers_Status_t status = fake_bus_record(fake, FAKE_BUS_READ, reg, staged, len);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    memcpy(buf, staged, len);
    return LIBDRIVERS_OK;
}

/**
 * @brief Write hook: store @p len bytes into the register file.
 *
 * A failing write does not modify the register file, so a test can prove a
 * driver stopped at the first failure rather than carrying on.
 */
static Libdrivers_Status_t fake_bus_write(void *ctx, uint8_t reg, const uint8_t *buf,
                                          uint16_t len) {
    FakeBus *fake = (FakeBus *)ctx;
    uint8_t base = reg & (uint8_t)~FAKE_BUS_AUTO_INCREMENT_BIT;

    Libdrivers_Status_t status = fake_bus_record(fake, FAKE_BUS_WRITE, reg, buf, len);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    for (uint16_t i = 0; i < len; i++) {
        fake->regs[(base + i) % FAKE_BUS_REG_COUNT] = buf[i];
    }

    return LIBDRIVERS_OK;
}

/** @brief Delay hook: record the call rather than actually sleeping. */
static void fake_bus_delay(void *ctx, uint32_t ms) {
    FakeBus *fake = (FakeBus *)ctx;
    fake->delay_calls++;
    fake->delay_total_ms += ms;
}

void fake_bus_init(FakeBus *fake, Libdrivers_Bus_t *bus) {
    memset(fake, 0, sizeof(*fake));

    bus->read = fake_bus_read;
    bus->write = fake_bus_write;
    bus->delay = fake_bus_delay;
    bus->ctx = fake;
}

void fake_bus_init_without_delay(FakeBus *fake, Libdrivers_Bus_t *bus) {
    fake_bus_init(fake, bus);
    bus->delay = NULL;
}

void fake_bus_fail_on_call(FakeBus *fake, size_t call_index, Libdrivers_Status_t status) {
    fake->fail_on_call = call_index;
    fake->fail_status = status;
}

void fake_bus_seed(FakeBus *fake, uint8_t reg, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        fake->regs[(reg + i) % FAKE_BUS_REG_COUNT] = data[i];
    }
}

const FakeBusTransaction *fake_bus_transaction(const FakeBus *fake, size_t index) {
    if (index >= fake->transaction_count || index >= FAKE_BUS_MAX_TRANSACTIONS) {
        return NULL;
    }
    return &fake->log[index];
}
