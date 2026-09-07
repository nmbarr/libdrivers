#include "fake_bus.h"
#include "libdrivers/icm42688.h"
#include "test_harness.h"

/**
 * @file test_icm42688.c
 * @brief Tests for the ICM-42688-P accelerometer + gyroscope driver.
 *
 * The odd one out among these drivers in three ways, each tested here: its
 * sample bytes are big-endian (high byte first) where the ST parts are
 * little-endian; Init has an ordering requirement (the config registers must
 * be written before PWR_MGMT0 powers the sub-sensors on) plus a mandatory
 * settle delay; and because that delay is mandatory, a bus with no delay hook
 * is an error rather than something to skip.
 */

/** @brief Wire a handle to a fresh fake. */
static void setup(ICM42688_Handle_t *handle, FakeBus *fake) {
    fake_bus_init(fake, &handle->bus);
}

/** @brief Seed a six-byte big-endian X/Y/Z block at @p reg. */
static void seed_axes(FakeBus *fake, uint8_t reg, int16_t x, int16_t y, int16_t z) {
    const uint8_t block[6] = {
        (uint8_t)((x >> 8) & 0xFF), (uint8_t)(x & 0xFF),        (uint8_t)((y >> 8) & 0xFF),
        (uint8_t)(y & 0xFF),        (uint8_t)((z >> 8) & 0xFF), (uint8_t)(z & 0xFF),
    };
    fake_bus_seed(fake, reg, block, sizeof(block));
}

/** @brief Init a handle with the given config bytes, asserting it succeeds. */
static void init_with(ICM42688_Handle_t *handle, FakeBus *fake, uint8_t accel_config0,
                      uint8_t gyro_config0) {
    setup(handle, fake);
    const ICM42688_Config_t config = {
        .PwrMgmt0 = 0x0F, .GyroConfig0 = gyro_config0, .AccelConfig0 = accel_config0};
    ICM42688_Init(handle, &config);
}

TEST(init_writes_config_registers_before_powering_the_sensors_on) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    const ICM42688_Config_t config = {.PwrMgmt0 = 0x0F, .GyroConfig0 = 0x06, .AccelConfig0 = 0x06};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_Init(&handle, &config));

    ASSERT_INT_EQ(3, fake.transaction_count);

    // Order is the point: full-scale and ODR must be set while the sub-sensors
    // are still off, so PWR_MGMT0 comes last.
    static const uint8_t expected_regs[] = {ICM42688_REG_GYRO_CONFIG0, ICM42688_REG_ACCEL_CONFIG0,
                                            ICM42688_REG_PWR_MGMT0};
    const uint8_t expected_values[] = {config.GyroConfig0, config.AccelConfig0, config.PwrMgmt0};

    for (size_t i = 0; i < 3; i++) {
        const FakeBusTransaction *write = fake_bus_transaction(&fake, i);
        ASSERT_TRUE(write != NULL);
        ASSERT_INT_EQ(FAKE_BUS_WRITE, write->op);
        ASSERT_INT_EQ(expected_regs[i], write->reg);
        ASSERT_INT_EQ(expected_values[i], write->data[0]);
    }
}

TEST(init_waits_for_the_power_mode_transition) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    const ICM42688_Config_t config = {.PwrMgmt0 = 0x0F, .GyroConfig0 = 0x06, .AccelConfig0 = 0x06};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_Init(&handle, &config));

    // PWR_MGMT0 needs 200 us to settle after an OFF->mode transition before
    // any further register access; the hook's granularity is milliseconds, so
    // the driver rounds up to one.
    ASSERT_INT_EQ(1, fake.delay_calls);
    ASSERT_TRUE(fake.delay_total_ms >= 1);
}

TEST(init_reports_a_missing_delay_hook_rather_than_skipping_the_wait) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    fake_bus_init_without_delay(&fake, &handle.bus);

    const ICM42688_Config_t config = {.PwrMgmt0 = 0x0F, .GyroConfig0 = 0x06, .AccelConfig0 = 0x06};

    // The delay hook is documented as optional in general, but this driver
    // genuinely needs it. Skipping the wait would leave the caller reading
    // registers the device has not finished bringing up.
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_ARG, ICM42688_Init(&handle, &config));
}

TEST(init_stops_at_the_first_failing_write) {
    for (size_t failing = 1; failing <= 3; failing++) {
        ICM42688_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        fake_bus_fail_on_call(&fake, failing, LIBDRIVERS_ERR_BUS);

        const ICM42688_Config_t config = {
            .PwrMgmt0 = 0x0F, .GyroConfig0 = 0x06, .AccelConfig0 = 0x06};
        ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, ICM42688_Init(&handle, &config));
        ASSERT_INT_EQ(failing, fake.transaction_count);

        // Bailing out before PWR_MGMT0 means there was no transition to wait for.
        ASSERT_INT_EQ(0, fake.delay_calls);
    }
}

TEST(read_reg_does_not_set_an_auto_increment_bit) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    uint8_t buffer[6] = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK,
                     ICM42688_ReadReg(&handle, ICM42688_REG_ACCEL_DATA_X1, buffer, sizeof(buffer)));

    // Like the LSM6DSL, this device auto-increments internally on a burst.
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(ICM42688_REG_ACCEL_DATA_X1, read->reg);
}

TEST(init_decodes_the_accelerometer_full_scale) {
    // ACCEL_FS_SEL[2:0] sits at bits [7:5] of ACCEL_CONFIG0; the ODR bits in
    // [3:0] are set too, so a decode that fails to mask them is caught.
    static const struct {
        uint8_t accel_config0;
        ICM42688_AccelFullScale_t expected;
    } vectors[] = {
        {0x06, ICM42688_ACCEL_FS_16G}, // FS_SEL = 000b
        {0x26, ICM42688_ACCEL_FS_8G},  // FS_SEL = 001b
        {0x46, ICM42688_ACCEL_FS_4G},  // FS_SEL = 010b
        {0x66, ICM42688_ACCEL_FS_2G},  // FS_SEL = 011b
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        ICM42688_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, vectors[i].accel_config0, 0x06);
        ASSERT_INT_EQ(vectors[i].expected, handle.AccelFullScale);
    }
}

TEST(init_decodes_the_gyroscope_full_scale) {
    // All eight GYRO_FS_SEL codes, including the three-bit ones that a
    // two-bit mask would truncate.
    static const struct {
        uint8_t gyro_config0;
        ICM42688_GyroFullScale_t expected;
    } vectors[] = {
        {0x06, ICM42688_GYRO_FS_2000},  {0x26, ICM42688_GYRO_FS_1000},
        {0x46, ICM42688_GYRO_FS_500},   {0x66, ICM42688_GYRO_FS_250},
        {0x86, ICM42688_GYRO_FS_125},   {0xA6, ICM42688_GYRO_FS_62_5},
        {0xC6, ICM42688_GYRO_FS_31_25}, {0xE6, ICM42688_GYRO_FS_15_625},
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        ICM42688_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, 0x06, vectors[i].gyro_config0);
        ASSERT_INT_EQ(vectors[i].expected, handle.GyroFullScale);
    }
}

TEST(read_raw_accel_decodes_big_endian_axes) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_axes(&fake, ICM42688_REG_ACCEL_DATA_X1, 1000, -2000, INT16_MIN);

    ICM42688_AccelData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_ReadRawAccel(&handle, &data));

    // High byte first, unlike every ST part in this library.
    ASSERT_INT_EQ(1000, data.X);
    ASSERT_INT_EQ(-2000, data.Y);
    ASSERT_INT_EQ(INT16_MIN, data.Z);
}

TEST(read_raw_accel_would_catch_a_byte_swap) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // 0x0102 read the wrong way round is 0x0201: an asymmetric value, so a
    // little-endian decode cannot coincidentally agree.
    static const uint8_t block[6] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00};
    fake_bus_seed(&fake, ICM42688_REG_ACCEL_DATA_X1, block, sizeof(block));

    ICM42688_AccelData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_ReadRawAccel(&handle, &data));
    ASSERT_INT_EQ(0x0102, data.X);
}

TEST(read_raw_gyro_reads_its_own_register_block) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // Accel output (0x1F) and gyro output (0x25) are adjacent blocks.
    seed_axes(&fake, ICM42688_REG_ACCEL_DATA_X1, 444, 555, 666);
    seed_axes(&fake, ICM42688_REG_GYRO_DATA_X1, 111, 222, 333);

    ICM42688_GyroData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_ReadRawGyro(&handle, &data));

    ASSERT_INT_EQ(111, data.X);
    ASSERT_INT_EQ(222, data.Y);
    ASSERT_INT_EQ(333, data.Z);

    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(ICM42688_REG_GYRO_DATA_X1, read->reg);
    ASSERT_INT_EQ(6, read->len);
}

TEST(read_accel_mg_scales_by_the_configured_full_scale) {
    // At 16384 counts, each range's LSB/g gives a round number of mg.
    static const struct {
        uint8_t accel_config0;
        int32_t expected_mg;
    } vectors[] = {
        {0x06, 8000}, // 16 g: 2048 LSB/g
        {0x26, 4000}, // 8 g:  4096 LSB/g
        {0x46, 2000}, // 4 g:  8192 LSB/g
        {0x66, 1000}, // 2 g:  16384 LSB/g
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        ICM42688_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, vectors[i].accel_config0, 0x06);
        seed_axes(&fake, ICM42688_REG_ACCEL_DATA_X1, 16384, -16384, 0);

        ICM42688_AccelData_mg_t data = {0};
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_ReadAccel_mg(&handle, &data));

        ASSERT_INT_EQ(vectors[i].expected_mg, data.X_mg);
        ASSERT_INT_EQ(-vectors[i].expected_mg, data.Y_mg);
        ASSERT_INT_EQ(0, data.Z_mg);
    }
}

TEST(read_gyro_mdps_scales_by_the_configured_full_scale) {
    // At 16384 counts, across all eight ranges. The table is LSB/dps x10,
    // and integer division truncates, so these are the exact expected values
    // rather than the ideal real-valued ones.
    static const struct {
        uint8_t gyro_config0;
        int32_t expected_mdps;
    } vectors[] = {
        {0x06, 999024}, // 2000 dps:   16.4 LSB/dps
        {0x26, 499512}, // 1000 dps:   32.8 LSB/dps
        {0x46, 250137}, // 500 dps:    65.5 LSB/dps
        {0x66, 125068}, // 250 dps:    131 LSB/dps
        {0x86, 62534},  // 125 dps:    262 LSB/dps
        {0xA6, 31249},  // 62.5 dps:   524.3 LSB/dps
        {0xC6, 15624},  // 31.25 dps:  1048.6 LSB/dps
        {0xE6, 7812},   // 15.625 dps: 2097.2 LSB/dps
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        ICM42688_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, 0x06, vectors[i].gyro_config0);
        seed_axes(&fake, ICM42688_REG_GYRO_DATA_X1, 16384, -16384, 0);

        ICM42688_GyroData_mdps_t data = {0};
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_ReadGyro_mdps(&handle, &data));

        ASSERT_INT_EQ(vectors[i].expected_mdps, data.X_mdps);
        ASSERT_INT_EQ(-vectors[i].expected_mdps, data.Y_mdps);
        ASSERT_INT_EQ(0, data.Z_mdps);
    }
}

TEST(read_temperature_applies_the_datasheet_formula) {
    // degC = (raw / 132.48) + 25, read big-endian from TEMP_DATA1.
    static const struct {
        int16_t raw;
        float expected;
    } vectors[] = {
        {0, 25.0f},        // the offset alone
        {1325, 35.0015f},  // ~10 degC above
        {-1325, 14.9985f}, // ~10 degC below; must sign-extend
        {-3312, 0.0f},     // roughly freezing
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        ICM42688_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);

        const uint8_t block[2] = {(uint8_t)((vectors[i].raw >> 8) & 0xFF),
                                  (uint8_t)(vectors[i].raw & 0xFF)};
        fake_bus_seed(&fake, ICM42688_REG_TEMP_DATA1, block, sizeof(block));

        float temperature = 0.0f;
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_ReadTemperature(&handle, &temperature));
        ASSERT_FLOAT_NEAR(vectors[i].expected, temperature, 0.01f);
    }
}

TEST(read_temperature_reads_two_bytes_from_temp_data1) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    float temperature = 0.0f;
    ICM42688_ReadTemperature(&handle, &temperature);

    ASSERT_INT_EQ(1, fake.transaction_count);
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(ICM42688_REG_TEMP_DATA1, read->reg);
    ASSERT_INT_EQ(2, read->len);
}

TEST(readers_propagate_transport_failures) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    init_with(&handle, &fake, 0x06, 0x06);

    // Call 4: the read that follows Init's three writes.
    fake_bus_fail_on_call(&fake, 4, LIBDRIVERS_ERR_TIMEOUT);

    ICM42688_AccelData_mg_t accel = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_TIMEOUT, ICM42688_ReadAccel_mg(&handle, &accel));

    ICM42688_Handle_t temperature_handle;
    FakeBus temperature_fake;
    setup(&temperature_handle, &temperature_fake);
    fake_bus_fail_on_call(&temperature_fake, 1, LIBDRIVERS_ERR_TIMEOUT);

    float temperature = 0.0f;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_TIMEOUT,
                     ICM42688_ReadTemperature(&temperature_handle, &temperature));
}

TEST(check_who_am_i_matches_the_datasheet_id) {
    ICM42688_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    fake.regs[ICM42688_REG_WHO_AM_I] = ICM42688_WHO_AM_I_VALUE;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, ICM42688_CheckWhoAmI(&handle));

    // 0x4E is the ICM-42605's -- same family, same register map layout,
    // different part.
    fake.regs[ICM42688_REG_WHO_AM_I] = 0x4E;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_ID, ICM42688_CheckWhoAmI(&handle));
}

static const LibdriversTest tests[] = {
    LIBDRIVERS_TEST(init_writes_config_registers_before_powering_the_sensors_on),
    LIBDRIVERS_TEST(init_waits_for_the_power_mode_transition),
    LIBDRIVERS_TEST(init_reports_a_missing_delay_hook_rather_than_skipping_the_wait),
    LIBDRIVERS_TEST(init_stops_at_the_first_failing_write),
    LIBDRIVERS_TEST(read_reg_does_not_set_an_auto_increment_bit),
    LIBDRIVERS_TEST(init_decodes_the_accelerometer_full_scale),
    LIBDRIVERS_TEST(init_decodes_the_gyroscope_full_scale),
    LIBDRIVERS_TEST(read_raw_accel_decodes_big_endian_axes),
    LIBDRIVERS_TEST(read_raw_accel_would_catch_a_byte_swap),
    LIBDRIVERS_TEST(read_raw_gyro_reads_its_own_register_block),
    LIBDRIVERS_TEST(read_accel_mg_scales_by_the_configured_full_scale),
    LIBDRIVERS_TEST(read_gyro_mdps_scales_by_the_configured_full_scale),
    LIBDRIVERS_TEST(read_temperature_applies_the_datasheet_formula),
    LIBDRIVERS_TEST(read_temperature_reads_two_bytes_from_temp_data1),
    LIBDRIVERS_TEST(readers_propagate_transport_failures),
    LIBDRIVERS_TEST(check_who_am_i_matches_the_datasheet_id),
};
LIBDRIVERS_TEST_MAIN("icm42688", tests)
