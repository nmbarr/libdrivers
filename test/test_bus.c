#include "fake_bus.h"
#include "libdrivers/bus.h"
#include "test_harness.h"

/**
 * @file test_bus.c
 * @brief Tests for Libdrivers_Bus_CheckWhoAmI, the shared identity helper.
 *
 * Every register-bus driver's CheckWhoAmI delegates here, so its three
 * outcomes -- right chip, wrong chip, broken transport -- are worth pinning
 * down once rather than six times.
 */

#define TEST_WHO_AM_I_REG   0x0F
#define TEST_WHO_AM_I_VALUE 0xBC

TEST(matching_id_returns_ok) {
    FakeBus fake;
    Libdrivers_Bus_t bus;
    fake_bus_init(&fake, &bus);
    fake.regs[TEST_WHO_AM_I_REG] = TEST_WHO_AM_I_VALUE;

    ASSERT_STATUS_EQ(LIBDRIVERS_OK,
                     Libdrivers_Bus_CheckWhoAmI(&bus, TEST_WHO_AM_I_REG, TEST_WHO_AM_I_VALUE));
}

TEST(mismatched_id_returns_err_id) {
    FakeBus fake;
    Libdrivers_Bus_t bus;
    fake_bus_init(&fake, &bus);

    // A chip that answers, but with the wrong identity: the bus is fine, so
    // this must not be reported as a transport failure.
    fake.regs[TEST_WHO_AM_I_REG] = 0x00;

    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_ID,
                     Libdrivers_Bus_CheckWhoAmI(&bus, TEST_WHO_AM_I_REG, TEST_WHO_AM_I_VALUE));
}

TEST(transport_error_propagates_unchanged) {
    FakeBus fake;
    Libdrivers_Bus_t bus;
    fake_bus_init(&fake, &bus);
    fake.regs[TEST_WHO_AM_I_REG] = TEST_WHO_AM_I_VALUE;

    // The read never happens, so the helper cannot know the ID matched. It
    // must hand back the transport's status rather than inventing ERR_ID.
    fake_bus_fail_on_call(&fake, 1, LIBDRIVERS_ERR_TIMEOUT);

    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_TIMEOUT,
                     Libdrivers_Bus_CheckWhoAmI(&bus, TEST_WHO_AM_I_REG, TEST_WHO_AM_I_VALUE));
}

TEST(reads_one_byte_from_the_plain_address) {
    FakeBus fake;
    Libdrivers_Bus_t bus;
    fake_bus_init(&fake, &bus);
    fake.regs[TEST_WHO_AM_I_REG] = TEST_WHO_AM_I_VALUE;

    Libdrivers_Bus_CheckWhoAmI(&bus, TEST_WHO_AM_I_REG, TEST_WHO_AM_I_VALUE);

    ASSERT_INT_EQ(1, fake.transaction_count);

    // A one-byte read has nothing to auto-increment over, so the helper goes
    // through the bus hook directly and the address must carry no extra bits.
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(FAKE_BUS_READ, read->op);
    ASSERT_INT_EQ(TEST_WHO_AM_I_REG, read->reg);
    ASSERT_INT_EQ(1, read->len);
}

static const LibdriversTest tests[] = {
    LIBDRIVERS_TEST(matching_id_returns_ok),
    LIBDRIVERS_TEST(mismatched_id_returns_err_id),
    LIBDRIVERS_TEST(transport_error_propagates_unchanged),
    LIBDRIVERS_TEST(reads_one_byte_from_the_plain_address),
};
LIBDRIVERS_TEST_MAIN("bus", tests)
