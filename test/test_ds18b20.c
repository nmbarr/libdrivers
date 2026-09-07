#include "fake_onewire.h"
#include "libdrivers/ds18b20.h"
#include "test_harness.h"

/**
 * @file test_ds18b20.c
 * @brief Tests for the DS18B20 1-Wire temperature driver.
 *
 * Two things matter here and neither is visible in a returned temperature:
 * the exact command sequence on the wire (a missing Skip ROM addresses
 * nothing on a multi-device line, and a missing reset desynchronises the
 * device), and the two's-complement decode of a negative reading.
 */

/** @brief Wire a handle to a fresh fake. */
static void setup(DS18B20_Handle_t *handle, FakeOneWire *fake) {
    fake_onewire_init(fake, &handle->ow);
}

TEST(init_issues_a_single_reset) {
    DS18B20_Handle_t handle;
    FakeOneWire fake;
    setup(&handle, &fake);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, DS18B20_Init(&handle));

    // Init is nothing but the presence handshake -- no commands follow it.
    ASSERT_INT_EQ(1, fake.reset_calls);
    ASSERT_INT_EQ(1, fake.step_count);
    ASSERT_INT_EQ(0, fake.write_count);
}

TEST(init_reports_no_device_on_the_line) {
    DS18B20_Handle_t handle;
    FakeOneWire fake;
    setup(&handle, &fake);

    // No presence pulse: the transport says ERR_BUS and that is the whole
    // answer, since a 1-Wire part has no ID register to fall back on.
    fake_onewire_fail_on_step(&fake, 1, LIBDRIVERS_ERR_BUS);

    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, DS18B20_Init(&handle));
}

TEST(start_conversion_sends_reset_skiprom_convert) {
    DS18B20_Handle_t handle;
    FakeOneWire fake;
    setup(&handle, &fake);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, DS18B20_StartConversion(&handle));

    ASSERT_INT_EQ(1, fake.reset_calls);
    ASSERT_INT_EQ(2, fake.write_count);
    ASSERT_INT_EQ(DS18B20_CMD_SKIP_ROM, fake.writes[0]);
    ASSERT_INT_EQ(DS18B20_CMD_CONVERT_T, fake.writes[1]);

    // Order is part of the protocol: the reset must come first, and Skip ROM
    // must precede the function command.
    ASSERT_INT_EQ(3, fake.step_count);
    ASSERT_INT_EQ(FAKE_ONEWIRE_RESET, fake_onewire_step(&fake, 0));
    ASSERT_INT_EQ(FAKE_ONEWIRE_WRITE, fake_onewire_step(&fake, 1));
    ASSERT_INT_EQ(FAKE_ONEWIRE_WRITE, fake_onewire_step(&fake, 2));
}

TEST(start_conversion_does_not_wait_for_the_conversion) {
    DS18B20_Handle_t handle;
    FakeOneWire fake;
    setup(&handle, &fake);

    DS18B20_StartConversion(&handle);

    // The documented split: a 12-bit conversion takes up to 750 ms, and this
    // call returns without reading anything back. The caller owns the delay.
    ASSERT_INT_EQ(0, fake.reads_served);
}

TEST(start_conversion_stops_at_the_first_failure) {
    // Failing each step in turn proves the driver bails immediately rather
    // than pushing the remaining commands at a device that is not listening.
    static const size_t expected_writes_after_failure[] = {0, 0, 1};

    for (size_t step = 1; step <= 3; step++) {
        DS18B20_Handle_t handle;
        FakeOneWire fake;
        setup(&handle, &fake);
        fake_onewire_fail_on_step(&fake, step, LIBDRIVERS_ERR_BUS);

        ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, DS18B20_StartConversion(&handle));
        ASSERT_INT_EQ(step, fake.step_count);
        ASSERT_INT_EQ(expected_writes_after_failure[step - 1], fake.write_count);
    }
}

TEST(read_temperature_sends_reset_skiprom_readscratchpad) {
    DS18B20_Handle_t handle;
    FakeOneWire fake;
    setup(&handle, &fake);

    static const uint8_t scratchpad[] = {0x91, 0x01}; // +25.0625 degC
    fake_onewire_script_reads(&fake, scratchpad, sizeof(scratchpad));

    float temperature = 0.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, DS18B20_ReadTemperature(&handle, &temperature));

    ASSERT_INT_EQ(1, fake.reset_calls);
    ASSERT_INT_EQ(2, fake.write_count);
    ASSERT_INT_EQ(DS18B20_CMD_SKIP_ROM, fake.writes[0]);
    ASSERT_INT_EQ(DS18B20_CMD_READ_SCRATCHPAD, fake.writes[1]);

    // Exactly the two temperature bytes; the driver does not read the rest of
    // the nine-byte scratchpad.
    ASSERT_INT_EQ(2, fake.reads_served);
}

TEST(read_temperature_decodes_the_datasheet_table) {
    // The temperature/data relationship table from the DS18B20 datasheet, at
    // the 12-bit default resolution the driver's scale factor assumes. Every
    // value is a multiple of 1/16, so binary float holds each one exactly and
    // these can be compared without a tolerance.
    static const struct {
        uint8_t lsb;
        uint8_t msb;
        float expected;
    } vectors[] = {
        {0xD0, 0x07, 125.0f},  {0x50, 0x05, 85.0f},    {0x91, 0x01, 25.0625f},
        {0xA2, 0x00, 10.125f}, {0x08, 0x00, 0.5f},     {0x00, 0x00, 0.0f},
        {0xF8, 0xFF, -0.5f},   {0x5E, 0xFF, -10.125f}, {0x6F, 0xFE, -25.0625f},
        {0x90, 0xFC, -55.0f},
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        DS18B20_Handle_t handle;
        FakeOneWire fake;
        setup(&handle, &fake);

        const uint8_t scratchpad[] = {vectors[i].lsb, vectors[i].msb};
        fake_onewire_script_reads(&fake, scratchpad, sizeof(scratchpad));

        float temperature = 12345.0f;
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, DS18B20_ReadTemperature(&handle, &temperature));
        ASSERT_FLOAT_NEAR(vectors[i].expected, temperature, 0.0f);
    }
}

TEST(read_temperature_reads_lsb_before_msb) {
    DS18B20_Handle_t handle;
    FakeOneWire fake;
    setup(&handle, &fake);

    // A byte-swapped decode would read 0x90FC (a large positive number) rather
    // than 0xFC90 (-55 degC), so this asserts the driver's byte order as much
    // as its sign handling.
    static const uint8_t scratchpad[] = {0x90, 0xFC};
    fake_onewire_script_reads(&fake, scratchpad, sizeof(scratchpad));

    float temperature = 0.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, DS18B20_ReadTemperature(&handle, &temperature));
    ASSERT_FLOAT_NEAR(-55.0f, temperature, 0.0f);
}

TEST(read_temperature_leaves_output_untouched_on_failure) {
    // Every step of the transaction, including each of the two data reads.
    for (size_t step = 1; step <= 5; step++) {
        DS18B20_Handle_t handle;
        FakeOneWire fake;
        setup(&handle, &fake);

        static const uint8_t scratchpad[] = {0x91, 0x01};
        fake_onewire_script_reads(&fake, scratchpad, sizeof(scratchpad));
        fake_onewire_fail_on_step(&fake, step, LIBDRIVERS_ERR_TIMEOUT);

        // A caller that ignores the status must not be handed a plausible
        // temperature; the documented contract is that this stays as it was.
        float temperature = -999.0f;
        ASSERT_STATUS_EQ(LIBDRIVERS_ERR_TIMEOUT, DS18B20_ReadTemperature(&handle, &temperature));
        ASSERT_FLOAT_NEAR(-999.0f, temperature, 0.0f);
    }
}

static const LibdriversTest tests[] = {
    LIBDRIVERS_TEST(init_issues_a_single_reset),
    LIBDRIVERS_TEST(init_reports_no_device_on_the_line),
    LIBDRIVERS_TEST(start_conversion_sends_reset_skiprom_convert),
    LIBDRIVERS_TEST(start_conversion_does_not_wait_for_the_conversion),
    LIBDRIVERS_TEST(start_conversion_stops_at_the_first_failure),
    LIBDRIVERS_TEST(read_temperature_sends_reset_skiprom_readscratchpad),
    LIBDRIVERS_TEST(read_temperature_decodes_the_datasheet_table),
    LIBDRIVERS_TEST(read_temperature_reads_lsb_before_msb),
    LIBDRIVERS_TEST(read_temperature_leaves_output_untouched_on_failure),
};
LIBDRIVERS_TEST_MAIN("ds18b20", tests)
