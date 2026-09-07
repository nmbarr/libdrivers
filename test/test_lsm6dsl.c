#include "fake_bus.h"
#include "libdrivers/lsm6dsl.h"
#include "test_harness.h"

/**
 * @file test_lsm6dsl.c
 * @brief Tests for the LSM6DSL accelerometer + gyroscope driver.
 *
 * Two things separate this driver from the simpler ST parts. It does NOT set
 * an auto-increment address bit -- the device auto-increments internally via
 * IF_INC -- so setting one would address the wrong register. And Init decodes
 * the configured full-scale out of the control bytes and caches it, which the
 * _mg/_mdps readers then index a sensitivity table with; a wrong decode picks
 * the wrong row and scales every sample by the wrong constant.
 */

/** @brief Wire a handle to a fresh fake. */
static void setup(LSM6DSL_Handle_t *handle, FakeBus *fake) {
    fake_bus_init(fake, &handle->bus);
}

/** @brief Seed a six-byte little-endian X/Y/Z block at @p reg. */
static void seed_axes(FakeBus *fake, uint8_t reg, int16_t x, int16_t y, int16_t z) {
    const uint8_t block[6] = {
        (uint8_t)(x & 0xFF),        (uint8_t)((x >> 8) & 0xFF), (uint8_t)(y & 0xFF),
        (uint8_t)((y >> 8) & 0xFF), (uint8_t)(z & 0xFF),        (uint8_t)((z >> 8) & 0xFF),
    };
    fake_bus_seed(fake, reg, block, sizeof(block));
}

/** @brief Init a handle with the given control bytes, asserting it succeeds. */
static void init_with(LSM6DSL_Handle_t *handle, FakeBus *fake, uint8_t ctrl1_xl, uint8_t ctrl2_g) {
    setup(handle, fake);
    const LSM6DSL_Config_t config = {
        .CtrlReg1_XL = ctrl1_xl, .CtrlReg2_G = ctrl2_g, .CtrlReg3_C = 0x44};
    LSM6DSL_Init(handle, &config);
}

TEST(init_writes_the_three_control_registers_in_order) {
    LSM6DSL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    const LSM6DSL_Config_t config = {.CtrlReg1_XL = 0x60, .CtrlReg2_G = 0x60, .CtrlReg3_C = 0x44};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LSM6DSL_Init(&handle, &config));

    ASSERT_INT_EQ(3, fake.transaction_count);

    static const uint8_t expected_regs[] = {LSM6DSL_REG_CTRL1_XL, LSM6DSL_REG_CTRL2_G,
                                            LSM6DSL_REG_CTRL3_C};
    const uint8_t expected_values[] = {config.CtrlReg1_XL, config.CtrlReg2_G, config.CtrlReg3_C};

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
        LSM6DSL_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        fake_bus_fail_on_call(&fake, failing, LIBDRIVERS_ERR_BUS);

        const LSM6DSL_Config_t config = {
            .CtrlReg1_XL = 0x60, .CtrlReg2_G = 0x60, .CtrlReg3_C = 0x44};
        ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, LSM6DSL_Init(&handle, &config));
        ASSERT_INT_EQ(failing, fake.transaction_count);
    }
}

TEST(read_reg_does_not_set_an_auto_increment_bit) {
    LSM6DSL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    uint8_t buffer[6] = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK,
                     LSM6DSL_ReadReg(&handle, LSM6DSL_REG_OUTX_L_XL, buffer, sizeof(buffer)));

    // The LSM6DSL auto-increments internally (IF_INC in CTRL3_C). Setting bit 7
    // in the address here would ask for register 0xA8, not 0x28.
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LSM6DSL_REG_OUTX_L_XL, read->reg);
    ASSERT_INT_EQ(6, read->len);
}

TEST(init_decodes_the_accelerometer_full_scale) {
    // FS_XL[1:0] sits at bits [3:2] of CTRL1_XL. The ODR bits above it must
    // not leak into the decode, so each case sets them too.
    static const struct {
        uint8_t ctrl1_xl;
        LSM6DSL_XLFullScale_t expected;
    } vectors[] = {
        {0x60, LSM6DSL_FS_XL_2G},  // ODR 416 Hz, FS_XL = 00b
        {0x64, LSM6DSL_FS_XL_16G}, // FS_XL = 01b
        {0x68, LSM6DSL_FS_XL_4G},  // FS_XL = 10b
        {0x6C, LSM6DSL_FS_XL_8G},  // FS_XL = 11b
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        LSM6DSL_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, vectors[i].ctrl1_xl, 0x00);
        ASSERT_INT_EQ(vectors[i].expected, handle.XLFullScale);
    }
}

TEST(init_decodes_the_gyroscope_full_scale) {
    static const struct {
        uint8_t ctrl2_g;
        LSM6DSL_GyroFullScale_t expected;
    } vectors[] = {
        {0x60, LSM6DSL_FS_G_250},  // ODR 416 Hz, FS_G = 00b
        {0x64, LSM6DSL_FS_G_500},  // FS_G = 01b
        {0x68, LSM6DSL_FS_G_1000}, // FS_G = 10b
        {0x6C, LSM6DSL_FS_G_2000}, // FS_G = 11b
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        LSM6DSL_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, 0x00, vectors[i].ctrl2_g);
        ASSERT_INT_EQ(vectors[i].expected, handle.GyroFullScale);
    }
}

TEST(init_lets_the_fs_125_bit_override_fs_g) {
    // FS_125 (bit 1) takes precedence over FS_G[1:0] in the hardware, so a
    // decode that checks FS_G first reports 2000 dps for a device actually
    // running at 125 -- a 16x scaling error with no other symptom.
    static const uint8_t ctrl2_g_values[] = {
        0x02, // FS_125 set, FS_G = 00b
        0x06, // FS_125 set, FS_G = 01b
        0x0E, // FS_125 set, FS_G = 11b -- the case that ordering gets wrong
    };

    for (size_t i = 0; i < sizeof(ctrl2_g_values) / sizeof(ctrl2_g_values[0]); i++) {
        LSM6DSL_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, 0x00, ctrl2_g_values[i]);
        ASSERT_INT_EQ(LSM6DSL_FS_G_125, handle.GyroFullScale);
    }

    // With FS_125 clear, FS_G decodes normally again.
    LSM6DSL_Handle_t handle;
    FakeBus fake;
    init_with(&handle, &fake, 0x00, 0x0C);
    ASSERT_INT_EQ(LSM6DSL_FS_G_2000, handle.GyroFullScale);
}

TEST(read_raw_xl_decodes_little_endian_axes) {
    LSM6DSL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_axes(&fake, LSM6DSL_REG_OUTX_L_XL, 1000, -2000, INT16_MIN);

    LSM6DSL_XLData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LSM6DSL_ReadRawXL(&handle, &data));

    ASSERT_INT_EQ(1000, data.X);
    ASSERT_INT_EQ(-2000, data.Y);
    ASSERT_INT_EQ(INT16_MIN, data.Z);

    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LSM6DSL_REG_OUTX_L_XL, read->reg);
}

TEST(read_raw_gyro_reads_its_own_register_block) {
    LSM6DSL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // Gyro output (0x22) sits just below accel output (0x28). Seed both with
    // different values so reading the wrong base register is caught.
    seed_axes(&fake, LSM6DSL_REG_OUTX_L_G, 111, 222, 333);
    seed_axes(&fake, LSM6DSL_REG_OUTX_L_XL, 444, 555, 666);

    LSM6DSL_GyroData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LSM6DSL_ReadRawGyro(&handle, &data));

    ASSERT_INT_EQ(111, data.X);
    ASSERT_INT_EQ(222, data.Y);
    ASSERT_INT_EQ(333, data.Z);

    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LSM6DSL_REG_OUTX_L_G, read->reg);
    ASSERT_INT_EQ(6, read->len);
}

TEST(read_xl_mg_scales_by_the_configured_full_scale) {
    // At 16384 counts the datasheet sensitivities give these mg values; the
    // driver's table is mg/LSB x1000, so the result is truncated integer mg.
    static const struct {
        uint8_t ctrl1_xl;
        int32_t expected_mg;
    } vectors[] = {
        {0x60, 999},  // 2 g:  0.061 mg/LSB
        {0x64, 7995}, // 16 g: 0.488 mg/LSB
        {0x68, 1998}, // 4 g:  0.122 mg/LSB
        {0x6C, 3997}, // 8 g:  0.244 mg/LSB
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        LSM6DSL_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, vectors[i].ctrl1_xl, 0x00);
        seed_axes(&fake, LSM6DSL_REG_OUTX_L_XL, 16384, -16384, 0);

        LSM6DSL_XLData_mg_t data = {0};
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, LSM6DSL_ReadXL_mg(&handle, &data));

        ASSERT_INT_EQ(vectors[i].expected_mg, data.X_mg);
        ASSERT_INT_EQ(-vectors[i].expected_mg, data.Y_mg); // symmetric about zero
        ASSERT_INT_EQ(0, data.Z_mg);
    }
}

TEST(read_gyro_mdps_scales_by_the_configured_full_scale) {
    // At 1000 counts. The table is mdps/LSB x8, which makes the datasheet's
    // 4.375/8.75/17.5 fractions exact, so these are not rounded.
    static const struct {
        uint8_t ctrl2_g;
        int32_t expected_mdps;
    } vectors[] = {
        {0x00, 8750},  // 250 dps:  8.75 mdps/LSB
        {0x04, 17500}, // 500 dps:  17.5 mdps/LSB
        {0x08, 35000}, // 1000 dps: 35 mdps/LSB
        {0x0C, 70000}, // 2000 dps: 70 mdps/LSB
        {0x02, 4375},  // 125 dps:  4.375 mdps/LSB, via the FS_125 bit
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        LSM6DSL_Handle_t handle;
        FakeBus fake;
        init_with(&handle, &fake, 0x00, vectors[i].ctrl2_g);
        seed_axes(&fake, LSM6DSL_REG_OUTX_L_G, 1000, -1000, 0);

        LSM6DSL_GyroData_mdps_t data = {0};
        ASSERT_STATUS_EQ(LIBDRIVERS_OK, LSM6DSL_ReadGyro_mdps(&handle, &data));

        ASSERT_INT_EQ(vectors[i].expected_mdps, data.X_mdps);
        ASSERT_INT_EQ(-vectors[i].expected_mdps, data.Y_mdps);
        ASSERT_INT_EQ(0, data.Z_mdps);
    }
}

TEST(scaled_readers_do_not_touch_the_bus_to_find_the_full_scale) {
    LSM6DSL_Handle_t handle;
    FakeBus fake;
    init_with(&handle, &fake, 0x64, 0x0C); // 16 g, 2000 dps
    seed_axes(&fake, LSM6DSL_REG_OUTX_L_XL, 16384, 0, 0);

    // Init's three writes are already on the log; a scaled read should add
    // exactly one more transaction, not re-read CTRL1_XL to find the range.
    size_t before = fake.transaction_count;

    LSM6DSL_XLData_mg_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LSM6DSL_ReadXL_mg(&handle, &data));

    ASSERT_INT_EQ(before + 1, fake.transaction_count);
    ASSERT_INT_EQ(7995, data.X_mg);
}

TEST(scaled_readers_propagate_transport_failures) {
    LSM6DSL_Handle_t xl_handle;
    FakeBus xl_fake;
    init_with(&xl_handle, &xl_fake, 0x60, 0x60);
    fake_bus_fail_on_call(&xl_fake, 4, LIBDRIVERS_ERR_BUS); // the read after Init's 3 writes

    LSM6DSL_XLData_mg_t xl_data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, LSM6DSL_ReadXL_mg(&xl_handle, &xl_data));

    LSM6DSL_Handle_t gyro_handle;
    FakeBus gyro_fake;
    init_with(&gyro_handle, &gyro_fake, 0x60, 0x60);
    fake_bus_fail_on_call(&gyro_fake, 4, LIBDRIVERS_ERR_BUS);

    LSM6DSL_GyroData_mdps_t gyro_data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, LSM6DSL_ReadGyro_mdps(&gyro_handle, &gyro_data));
}

TEST(check_who_am_i_matches_the_datasheet_id) {
    LSM6DSL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    fake.regs[LSM6DSL_REG_WHO_AM_I] = LSM6DSL_WHO_AM_I_VALUE;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LSM6DSL_CheckWhoAmI(&handle));

    // 0x6C is the LSM6DSO's -- the successor part, pin-compatible enough to be
    // fitted by mistake and register-incompatible enough to matter.
    fake.regs[LSM6DSL_REG_WHO_AM_I] = 0x6C;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_ID, LSM6DSL_CheckWhoAmI(&handle));
}

static const LibdriversTest tests[] = {
    LIBDRIVERS_TEST(init_writes_the_three_control_registers_in_order),
    LIBDRIVERS_TEST(init_stops_at_the_first_failing_write),
    LIBDRIVERS_TEST(read_reg_does_not_set_an_auto_increment_bit),
    LIBDRIVERS_TEST(init_decodes_the_accelerometer_full_scale),
    LIBDRIVERS_TEST(init_decodes_the_gyroscope_full_scale),
    LIBDRIVERS_TEST(init_lets_the_fs_125_bit_override_fs_g),
    LIBDRIVERS_TEST(read_raw_xl_decodes_little_endian_axes),
    LIBDRIVERS_TEST(read_raw_gyro_reads_its_own_register_block),
    LIBDRIVERS_TEST(read_xl_mg_scales_by_the_configured_full_scale),
    LIBDRIVERS_TEST(read_gyro_mdps_scales_by_the_configured_full_scale),
    LIBDRIVERS_TEST(scaled_readers_do_not_touch_the_bus_to_find_the_full_scale),
    LIBDRIVERS_TEST(scaled_readers_propagate_transport_failures),
    LIBDRIVERS_TEST(check_who_am_i_matches_the_datasheet_id),
};
LIBDRIVERS_TEST_MAIN("lsm6dsl", tests)
