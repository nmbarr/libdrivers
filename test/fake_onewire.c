#include "fake_onewire.h"
#include <string.h>

/**
 * @brief Record a step and decide whether this call should fail.
 *
 * Counts every hook call so an injected failure lands on the step the test
 * named, whichever hook that step goes through.
 */
static Libdrivers_Status_t fake_onewire_record(FakeOneWire *fake, FakeOneWireOp op) {
    if (fake->step_count < sizeof(fake->steps) / sizeof(fake->steps[0])) {
        fake->steps[fake->step_count] = op;
    }
    fake->step_count++;

    if (fake->fail_on_step != 0 && fake->step_count == fake->fail_on_step) {
        return fake->fail_status;
    }

    return LIBDRIVERS_OK;
}

/** @brief Reset hook: count the presence-detect handshake. */
static Libdrivers_Status_t fake_onewire_reset(void *ctx) {
    FakeOneWire *fake = (FakeOneWire *)ctx;
    fake->reset_calls++;
    return fake_onewire_record(fake, FAKE_ONEWIRE_RESET);
}

/**
 * @brief Write hook: record the command byte.
 *
 * A failing write is still recorded as attempted but its byte is not stored,
 * so a test can tell "the driver tried this command" from "the command landed".
 */
static Libdrivers_Status_t fake_onewire_write(void *ctx, uint8_t byte) {
    FakeOneWire *fake = (FakeOneWire *)ctx;

    Libdrivers_Status_t status = fake_onewire_record(fake, FAKE_ONEWIRE_WRITE);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    if (fake->write_count < FAKE_ONEWIRE_MAX_WRITES) {
        fake->writes[fake->write_count] = byte;
    }
    fake->write_count++;

    return LIBDRIVERS_OK;
}

/**
 * @brief Read hook: serve the next scripted byte.
 *
 * Leaves @p byte untouched on failure, and once the script runs out -- the
 * contract says the destination is written only on LIBDRIVERS_OK, and a driver
 * that reads more bytes than the device has is a bug the test should catch
 * rather than have papered over with a default value.
 */
static Libdrivers_Status_t fake_onewire_read(void *ctx, uint8_t *byte) {
    FakeOneWire *fake = (FakeOneWire *)ctx;

    Libdrivers_Status_t status = fake_onewire_record(fake, FAKE_ONEWIRE_READ);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    if (fake->reads_served >= fake->read_script_len) {
        return LIBDRIVERS_ERR_BUS;
    }

    *byte = fake->read_script[fake->reads_served];
    fake->reads_served++;

    return LIBDRIVERS_OK;
}

void fake_onewire_init(FakeOneWire *fake, Libdrivers_OneWire_t *ow) {
    memset(fake, 0, sizeof(*fake));

    ow->reset = fake_onewire_reset;
    ow->write = fake_onewire_write;
    ow->read = fake_onewire_read;
    ow->ctx = fake;
}

void fake_onewire_script_reads(FakeOneWire *fake, const uint8_t *data, size_t len) {
    size_t copy = len < FAKE_ONEWIRE_MAX_READS ? len : FAKE_ONEWIRE_MAX_READS;
    memcpy(fake->read_script, data, copy);
    fake->read_script_len = copy;
    fake->reads_served = 0;
}

void fake_onewire_fail_on_step(FakeOneWire *fake, size_t step_index, Libdrivers_Status_t status) {
    fake->fail_on_step = step_index;
    fake->fail_status = status;
}

FakeOneWireOp fake_onewire_step(const FakeOneWire *fake, size_t index) {
    if (index >= fake->step_count) {
        return FAKE_ONEWIRE_RESET;
    }
    return fake->steps[index];
}
