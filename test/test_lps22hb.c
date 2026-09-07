#include "fake_bus.h"
#include "libdrivers/lps22hb.h"
#include "test_harness.h"

/**
 * @file test_lps22hb.c
 * @brief Tests for the LPS22HB pressure + temperature driver.
 *
 * Pressure is the awkward one: a 24-bit two's-complement value spread over
 * three registers and widened into an int32_t. Sign-extending it wrongly turns
 * a small negative reading into roughly +4096 hPa, which is not obviously
 * wrong in a log -- so the negative cases are tested explicitly.
 */

/** @brief Wire a handle to a fresh fake. */
static void setup(LPS22HB_Handle_t *handle, FakeBus *fake) {
    fake_bus_init(fake, &handle->bus);
}

/**
 * @brief Seed the output block (0x28..0x2C).
 *
 * @param pressure_raw    24-bit pressure count; only the low 24 bits are used.
 * @param temperature_raw 16-bit temperature count.
 */
static void seed_output(FakeBus *fake, int32_t pressure_raw, int16_t temperature_raw) {
    const uint8_t out[5] = {
        (uint8_t)(pressure_raw & 0xFF),           // PRESS_OUT_XL
        (uint8_t)((pressure_raw >> 8) & 0xFF),    // PRESS_OUT_L
        (uint8_t)((pressure_raw >> 16) & 0xFF),   // PRESS_OUT_H
        (uint8_t)(temperature_raw & 0xFF),        // TEMP_OUT_L
        (uint8_t)((temperature_raw >> 8) & 0xFF), // TEMP_OUT_H
    };
    fake_bus_seed(fake, LPS22HB_REG_PRESS_OUT_XL, out, sizeof(out));
}

TEST(init_writes_the_three_control_registers_in_order) {
    LPS22HB_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    const LPS22HB_Config_t config = {.CtrlReg1 = 0x30, .CtrlReg2 = 0x10, .CtrlReg3 = 0x00};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_Init(&handle, &config));

    ASSERT_INT_EQ(3, fake.transaction_count);

    static const uint8_t expected_regs[] = {LPS22HB_REG_CTRL_REG1, LPS22HB_REG_CTRL_REG2,
                                            LPS22HB_REG_CTRL_REG3};
    const uint8_t expected_values[] = {config.CtrlReg1, config.CtrlReg2, config.CtrlReg3};

    for (size_t i = 0; i < 3; i++) {
        const FakeBusTransaction *write = fake_bus_transaction(&fake, i);
        ASSERT_TRUE(write != NULL);
        ASSERT_INT_EQ(FAKE_BUS_WRITE, write->op);
        ASSERT_INT_EQ(expected_regs[i], write->reg);
        ASSERT_INT_EQ(expected_values[i], write->data[0]);
    }
}

TEST(init_stops_at_the_first_failing_write) {
    for (size_t failing = 1; failing <= 3; failing++) {
        LPS22HB_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        fake_bus_fail_on_call(&fake, failing, LIBDRIVERS_ERR_BUS);

        const LPS22HB_Config_t config = {.CtrlReg1 = 0x30, .CtrlReg2 = 0x10, .CtrlReg3 = 0x01};
        ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, LPS22HB_Init(&handle, &config));
        ASSERT_INT_EQ(failing, fake.transaction_count);
    }
}

TEST(read_reg_sets_the_auto_increment_bit) {
    LPS22HB_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    uint8_t buffer[5] = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK,
                     LPS22HB_ReadReg(&handle, LPS22HB_REG_PRESS_OUT_XL, buffer, sizeof(buffer)));

    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LPS22HB_REG_PRESS_OUT_XL | LPS22HB_AUTO_INCREMENT_BIT, read->reg);
}

TEST(read_raw_output_reads_five_bytes_in_one_burst) {
    LPS22HB_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_output(&fake, 4150272, 2350);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_ReadRawOutput(&handle));

    // Pressure and temperature are adjacent, so one burst covers both.
    ASSERT_INT_EQ(1, fake.transaction_count);
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LPS22HB_REG_PRESS_OUT_XL | LPS22HB_AUTO_INCREMENT_BIT, read->reg);
    ASSERT_INT_EQ(5, read->len);
}

TEST(read_raw_output_assembles_the_24_bit_pressure) {
    LPS22HB_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // 1013.25 hPa at 4096 counts/hPa -- standard sea-level pressure.
    seed_output(&fake, 4150272, 0);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_ReadRawOutput(&handle));
    ASSERT_INT_EQ(4150272, handle.P_OUT);
}

TEST(read_raw_output_sign_extends_negative_pressure) {
    LPS22HB_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // A below-reference reading. Widening the 24-bit value without extending
    // the sign gives 0x00FFF000 = 16773120 instead of -4096.
    seed_output(&fake, -4096, 0);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_ReadRawOutput(&handle));
    ASSERT_INT_EQ(-4096, handle.P_OUT);
}

TEST(read_raw_output_handles_the_24_bit_extremes) {
    static const struct {
        int32_t raw;
        int32_t expected;
    } vectors[] = {
        {0x000000, 0},        // zero
        {0x7FFFFF, 8388607},  // largest positive 24-bit value
        {0x800000, -8388608}, // most negative: the sign bit alone
        {0xFFFFFF, -1},       // all ones
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        LPS22HB_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        seed_output(&fake, vectors[i].raw, 0);

        ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_ReadRawOutput(&handle));
        ASSERT_INT_EQ(vectors[i].expected, handle.P_OUT);
    }
}

TEST(read_raw_output_sign_extends_negative_temperature) {
    LPS22HB_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_output(&fake, 0, -1000);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_ReadRawOutput(&handle));
    ASSERT_INT_EQ(-1000, handle.T_OUT);
}

TEST(read_pressure_scales_to_hpa) {
    static const struct {
        int32_t raw;
        float expected;
    } vectors[] = {
        {4150272, 1013.25f},                    // standard sea level
        {4096000, 1000.0f},  {1064960, 260.0f}, // roughly the top of the device's range
        {0, 0.0f},           {-4096, -1.0f},    // below the reference pressure
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        LPS22HB_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        seed_output(&fake, vectors[i].raw, 0);

        float pressure = 0.0f;
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_ReadPressure(&handle, &pressure));
        ASSERT_FLOAT_NEAR(vectors[i].expected, pressure, 0.001f);
    }
}

TEST(read_temperature_scales_to_degc) {
    static const struct {
        int16_t raw;
        float expected;
    } vectors[] = {
        {2350, 23.5f}, {0, 0.0f}, {-1000, -10.0f}, {8500, 85.0f}, // top of the operating range
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        LPS22HB_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        seed_output(&fake, 0, vectors[i].raw);

        float temperature = 0.0f;
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_ReadTemperature(&handle, &temperature));
        ASSERT_FLOAT_NEAR(vectors[i].expected, temperature, 0.001f);
    }
}

TEST(readers_propagate_transport_failures) {
    LPS22HB_Handle_t pressure_handle;
    FakeBus pressure_fake;
    setup(&pressure_handle, &pressure_fake);
    fake_bus_fail_on_call(&pressure_fake, 1, LIBDRIVERS_ERR_TIMEOUT);

    float pressure = 0.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_TIMEOUT, LPS22HB_ReadPressure(&pressure_handle, &pressure));

    LPS22HB_Handle_t temperature_handle;
    FakeBus temperature_fake;
    setup(&temperature_handle, &temperature_fake);
    fake_bus_fail_on_call(&temperature_fake, 1, LIBDRIVERS_ERR_TIMEOUT);

    float temperature = 0.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_TIMEOUT,
                     LPS22HB_ReadTemperature(&temperature_handle, &temperature));
}

TEST(check_who_am_i_matches_the_datasheet_id) {
    LPS22HB_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    fake.regs[LPS22HB_REG_WHO_AM_I] = LPS22HB_WHO_AM_I_VALUE;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LPS22HB_CheckWhoAmI(&handle));

    // 0xBD is the LPS22HH's ID -- the closest neighbouring part, and the most
    // likely thing to be soldered down by mistake.
    fake.regs[LPS22HB_REG_WHO_AM_I] = 0xBD;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_ID, LPS22HB_CheckWhoAmI(&handle));
}

static const LibdriversTest tests[] = {
    LIBDRIVERS_TEST(init_writes_the_three_control_registers_in_order),
    LIBDRIVERS_TEST(init_stops_at_the_first_failing_write),
    LIBDRIVERS_TEST(read_reg_sets_the_auto_increment_bit),
    LIBDRIVERS_TEST(read_raw_output_reads_five_bytes_in_one_burst),
    LIBDRIVERS_TEST(read_raw_output_assembles_the_24_bit_pressure),
    LIBDRIVERS_TEST(read_raw_output_sign_extends_negative_pressure),
    LIBDRIVERS_TEST(read_raw_output_handles_the_24_bit_extremes),
    LIBDRIVERS_TEST(read_raw_output_sign_extends_negative_temperature),
    LIBDRIVERS_TEST(read_pressure_scales_to_hpa),
    LIBDRIVERS_TEST(read_temperature_scales_to_degc),
    LIBDRIVERS_TEST(readers_propagate_transport_failures),
    LIBDRIVERS_TEST(check_who_am_i_matches_the_datasheet_id),
};
LIBDRIVERS_TEST_MAIN("lps22hb", tests)
