#include "fake_bus.h"
#include "libdrivers/hts221.h"
#include "test_harness.h"

/**
 * @file test_hts221.c
 * @brief Tests for the HTS221 humidity + temperature driver.
 *
 * The interesting part is HTS221_ReadCalibration: sixteen bytes of factory
 * data in which the two temperature calibration points are 10-bit values with
 * their high bits packed into one shared register. Getting that unpack wrong
 * skews every subsequent reading rather than failing outright, so it is
 * pinned down here byte by byte.
 */

/** @brief Wire a handle to a fresh fake. */
static void setup(HTS221_Handle_t *handle, FakeBus *fake) {
    fake_bus_init(fake, &handle->bus);
}

/**
 * @brief Seed the calibration block (0x30..0x3F) with a known set of points.
 *
 * Chosen so the interpolation lands on exact values:
 *   T0 = 240/8 = 30 degC at raw 0      T1 = 480/8 = 60 degC at raw 1000
 *   H0 =  40/2 = 20 %RH  at raw 0      H1 = 160/2 = 80 %RH  at raw 600
 *
 * T1 = 480 = 0x1E0 needs bit 8 set, so it exercises the shared-MSB register;
 * T0 = 240 = 0x0F0 does not, so a decode that confuses the two fields shows up.
 */
static void seed_calibration(FakeBus *fake) {
    static const uint8_t cal[16] = {
        [0x30 - 0x30] = 40,   // H0_rH_x2       -> 20 %RH
        [0x31 - 0x30] = 160,  // H1_rH_x2       -> 80 %RH
        [0x32 - 0x30] = 0xF0, // T0_degC_x8 low  (240 = 0x0F0)
        [0x33 - 0x30] = 0xE0, // T1_degC_x8 low  (480 = 0x1E0)
        [0x35 - 0x30] = 0x04, // T1/T0 MSBs: T0[9:8]=00b, T1[9:8]=01b
        [0x36 - 0x30] = 0x00, // H0_T0_OUT = 0
        [0x37 - 0x30] = 0x00,
        [0x3A - 0x30] = 0x58, // H1_T0_OUT = 600 = 0x0258
        [0x3B - 0x30] = 0x02,
        [0x3C - 0x30] = 0x00, // T0_OUT = 0
        [0x3D - 0x30] = 0x00,
        [0x3E - 0x30] = 0xE8, // T1_OUT = 1000 = 0x03E8
        [0x3F - 0x30] = 0x03,
    };
    fake_bus_seed(fake, HTS221_REG_H0_RH_X2, cal, sizeof(cal));
}

/** @brief Seed the live output block (0x28..0x2B). */
static void seed_output(FakeBus *fake, int16_t humidity_raw, int16_t temperature_raw) {
    const uint8_t out[4] = {
        (uint8_t)(humidity_raw & 0xFF),
        (uint8_t)((humidity_raw >> 8) & 0xFF),
        (uint8_t)(temperature_raw & 0xFF),
        (uint8_t)((temperature_raw >> 8) & 0xFF),
    };
    fake_bus_seed(fake, HTS221_REG_HUMIDITY_OUT_L, out, sizeof(out));
}

TEST(init_writes_the_three_control_registers_in_order) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    const HTS221_Config_t config = {.CtrlReg1 = 0x85, .CtrlReg2 = 0x00, .CtrlReg3 = 0x04};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_Init(&handle, &config));

    ASSERT_INT_EQ(3, fake.transaction_count);

    static const uint8_t expected_regs[] = {HTS221_REG_CTRL_REG1, HTS221_REG_CTRL_REG2,
                                            HTS221_REG_CTRL_REG3};
    const uint8_t expected_values[] = {config.CtrlReg1, config.CtrlReg2, config.CtrlReg3};

    for (size_t i = 0; i < 3; i++) {
        const FakeBusTransaction *write = fake_bus_transaction(&fake, i);
        ASSERT_TRUE(write != NULL);
        ASSERT_INT_EQ(FAKE_BUS_WRITE, write->op);
        ASSERT_INT_EQ(expected_regs[i], write->reg);
        ASSERT_INT_EQ(1, write->len);
        ASSERT_INT_EQ(expected_values[i], write->data[0]);
    }
}

TEST(init_stops_at_the_first_failing_write) {
    for (size_t failing = 1; failing <= 3; failing++) {
        HTS221_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        fake_bus_fail_on_call(&fake, failing, LIBDRIVERS_ERR_BUS);

        const HTS221_Config_t config = {.CtrlReg1 = 0x85, .CtrlReg2 = 0x01, .CtrlReg3 = 0x04};
        ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, HTS221_Init(&handle, &config));

        // Half-configuring a sensor and carrying on would leave it sampling
        // with a mix of new and default settings.
        ASSERT_INT_EQ(failing, fake.transaction_count);
    }
}

TEST(read_reg_sets_the_auto_increment_bit) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    uint8_t buffer[4] = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK,
                     HTS221_ReadReg(&handle, HTS221_REG_HUMIDITY_OUT_L, buffer, sizeof(buffer)));

    // Without bit 7 the HTS221 returns the same register four times instead of
    // walking the block, and the decoded sample is silently wrong.
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(HTS221_REG_HUMIDITY_OUT_L | HTS221_AUTO_INCREMENT_BIT, read->reg);
}

TEST(write_reg_does_not_set_the_auto_increment_bit) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_WriteReg(&handle, HTS221_REG_CTRL_REG1, 0x85));

    // A single-byte write has nothing to increment over.
    const FakeBusTransaction *write = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(write != NULL);
    ASSERT_INT_EQ(HTS221_REG_CTRL_REG1, write->reg);
    ASSERT_INT_EQ(1, write->len);
    ASSERT_INT_EQ(0x85, write->data[0]);
}

TEST(read_calibration_unpacks_every_field) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_calibration(&fake);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadCalibration(&handle));

    ASSERT_INT_EQ(40, handle.H0_rH_x2);
    ASSERT_INT_EQ(160, handle.H1_rH_x2);

    // The 10-bit temperature points: low byte from its own register, high two
    // bits out of the shared 0x35. T0 takes bits [1:0], T1 bits [3:2].
    ASSERT_INT_EQ(240, handle.T0_degC_x8);
    ASSERT_INT_EQ(480, handle.T1_degC_x8);

    ASSERT_INT_EQ(0, handle.H0_T0_OUT);
    ASSERT_INT_EQ(600, handle.H1_T0_OUT);
    ASSERT_INT_EQ(0, handle.T0_OUT);
    ASSERT_INT_EQ(1000, handle.T1_OUT);
}

TEST(read_calibration_reads_the_whole_block_in_one_burst) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_calibration(&fake);

    HTS221_ReadCalibration(&handle);

    // One 16-byte burst from 0x30 with auto-increment, not sixteen reads.
    ASSERT_INT_EQ(1, fake.transaction_count);
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(HTS221_REG_H0_RH_X2 | HTS221_AUTO_INCREMENT_BIT, read->reg);
    ASSERT_INT_EQ(16, read->len);
}

TEST(read_calibration_decodes_the_full_ten_bit_range) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // Both fields at their maximum, with all four MSB bits set: a mask that is
    // too wide or a shift in the wrong direction cannot survive this.
    uint8_t cal[16] = {0};
    cal[HTS221_REG_T0_DEGC_X8 - HTS221_REG_H0_RH_X2] = 0xFF;
    cal[HTS221_REG_T1_DEGC_X8 - HTS221_REG_H0_RH_X2] = 0xFF;
    cal[HTS221_REG_T0_T1_MSB - HTS221_REG_H0_RH_X2] = 0x0F;
    fake_bus_seed(&fake, HTS221_REG_H0_RH_X2, cal, sizeof(cal));

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadCalibration(&handle));
    ASSERT_INT_EQ(1023, handle.T0_degC_x8);
    ASSERT_INT_EQ(1023, handle.T1_degC_x8);
}

TEST(read_calibration_ignores_bits_outside_its_fields) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // Bits [7:4] of 0x35 are not part of either temperature point. Leaking
    // them in would corrupt both readings.
    uint8_t cal[16] = {0};
    cal[HTS221_REG_T0_DEGC_X8 - HTS221_REG_H0_RH_X2] = 0x10;
    cal[HTS221_REG_T1_DEGC_X8 - HTS221_REG_H0_RH_X2] = 0x20;
    cal[HTS221_REG_T0_T1_MSB - HTS221_REG_H0_RH_X2] = 0xF0;
    fake_bus_seed(&fake, HTS221_REG_H0_RH_X2, cal, sizeof(cal));

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadCalibration(&handle));
    ASSERT_INT_EQ(0x10, handle.T0_degC_x8);
    ASSERT_INT_EQ(0x20, handle.T1_degC_x8);
}

TEST(read_raw_output_combines_little_endian_pairs) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_output(&fake, 500, -1200);

    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadRawOutput(&handle));

    ASSERT_INT_EQ(500, handle.H_OUT);
    ASSERT_INT_EQ(-1200, handle.T_OUT); // Negative raw counts must sign-extend

    // One 4-byte burst from HUMIDITY_OUT_L covers both samples.
    ASSERT_INT_EQ(1, fake.transaction_count);
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(HTS221_REG_HUMIDITY_OUT_L | HTS221_AUTO_INCREMENT_BIT, read->reg);
    ASSERT_INT_EQ(4, read->len);
}

TEST(read_temperature_interpolates_between_calibration_points) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_calibration(&fake);
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadCalibration(&handle));

    // T0 = 30 degC at raw 0, T1 = 60 degC at raw 1000.
    static const struct {
        int16_t raw;
        float expected;
    } vectors[] = {
        {0, 30.0f},    // exactly the low calibration point
        {1000, 60.0f}, // exactly the high calibration point
        {500, 45.0f},  // midway
        {2000, 90.0f}, // extrapolated above T1
        {-1000, 0.0f}, // extrapolated below T0
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        seed_output(&fake, 0, vectors[i].raw);

        float temperature = 0.0f;
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadTemperature(&handle, &temperature));
        ASSERT_FLOAT_NEAR(vectors[i].expected, temperature, 0.001f);
    }
}

TEST(read_humidity_interpolates_between_calibration_points) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_calibration(&fake);
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadCalibration(&handle));

    // H0 = 20 %RH at raw 0, H1 = 80 %RH at raw 600.
    static const struct {
        int16_t raw;
        float expected;
    } vectors[] = {
        {0, 20.0f},
        {600, 80.0f},
        {300, 50.0f},
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        seed_output(&fake, vectors[i].raw, 0);

        float humidity = 0.0f;
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadHumidity(&handle, &humidity));
        ASSERT_FLOAT_NEAR(vectors[i].expected, humidity, 0.001f);
    }
}

TEST(calibrated_readers_take_a_fresh_sample_each_call) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_calibration(&fake);
    HTS221_ReadCalibration(&handle);

    seed_output(&fake, 300, 500);
    float first = 0.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadTemperature(&handle, &first));

    // The device has moved on; a cached raw sample would return the old value.
    seed_output(&fake, 300, 1000);
    float second = 0.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_ReadTemperature(&handle, &second));

    ASSERT_FLOAT_NEAR(45.0f, first, 0.001f);
    ASSERT_FLOAT_NEAR(60.0f, second, 0.001f);
}

TEST(read_temperature_leaves_output_untouched_on_failure) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_calibration(&fake);
    HTS221_ReadCalibration(&handle);
    seed_output(&fake, 300, 500);

    // Fail the raw read the calibrated reader performs. Call 2, because the
    // calibration burst above was call 1.
    fake_bus_fail_on_call(&fake, 2, LIBDRIVERS_ERR_BUS);

    // A caller that ignores the status must not see a plausible temperature.
    float temperature = -999.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, HTS221_ReadTemperature(&handle, &temperature));
    ASSERT_FLOAT_NEAR(-999.0f, temperature, 0.0f);
}

TEST(read_humidity_leaves_output_untouched_on_failure) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_calibration(&fake);
    HTS221_ReadCalibration(&handle);
    seed_output(&fake, 300, 500);

    fake_bus_fail_on_call(&fake, 2, LIBDRIVERS_ERR_BUS);

    float humidity = -999.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, HTS221_ReadHumidity(&handle, &humidity));
    ASSERT_FLOAT_NEAR(-999.0f, humidity, 0.0f);
}

TEST(check_who_am_i_matches_the_datasheet_id) {
    HTS221_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    fake.regs[HTS221_REG_WHO_AM_I] = HTS221_WHO_AM_I_VALUE;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, HTS221_CheckWhoAmI(&handle));

    // 0xBC is the HTS221's. 0x3D is the LIS3MDL's -- a plausible neighbour on
    // the same bus, and exactly the confusion this check exists to catch.
    fake.regs[HTS221_REG_WHO_AM_I] = 0x3D;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_ID, HTS221_CheckWhoAmI(&handle));
}

static const LibdriversTest tests[] = {
    LIBDRIVERS_TEST(init_writes_the_three_control_registers_in_order),
    LIBDRIVERS_TEST(init_stops_at_the_first_failing_write),
    LIBDRIVERS_TEST(read_reg_sets_the_auto_increment_bit),
    LIBDRIVERS_TEST(write_reg_does_not_set_the_auto_increment_bit),
    LIBDRIVERS_TEST(read_calibration_unpacks_every_field),
    LIBDRIVERS_TEST(read_calibration_reads_the_whole_block_in_one_burst),
    LIBDRIVERS_TEST(read_calibration_decodes_the_full_ten_bit_range),
    LIBDRIVERS_TEST(read_calibration_ignores_bits_outside_its_fields),
    LIBDRIVERS_TEST(read_raw_output_combines_little_endian_pairs),
    LIBDRIVERS_TEST(read_temperature_interpolates_between_calibration_points),
    LIBDRIVERS_TEST(read_humidity_interpolates_between_calibration_points),
    LIBDRIVERS_TEST(calibrated_readers_take_a_fresh_sample_each_call),
    LIBDRIVERS_TEST(read_temperature_leaves_output_untouched_on_failure),
    LIBDRIVERS_TEST(read_humidity_leaves_output_untouched_on_failure),
    LIBDRIVERS_TEST(check_who_am_i_matches_the_datasheet_id),
};
LIBDRIVERS_TEST_MAIN("hts221", tests)
