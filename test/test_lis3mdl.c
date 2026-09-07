#include "fake_bus.h"
#include "libdrivers/lis3mdl.h"
#include "test_harness.h"

/**
 * @file test_lis3mdl.c
 * @brief Tests for the LIS3MDL 3-axis magnetometer driver.
 *
 * Axis data and the hard-iron offsets are both six-byte little-endian blocks,
 * read from different base registers. Reading the right bytes from the wrong
 * register, or pairing the bytes the wrong way round, both produce plausible
 * numbers -- so both the address and the decode are asserted.
 */

/** @brief Wire a handle to a fresh fake. */
static void setup(LIS3MDL_Handle_t *handle, FakeBus *fake) {
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

TEST(init_writes_the_five_control_registers_in_order) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    const LIS3MDL_Config_t config = {
        .CtrlReg1 = 0x70, .CtrlReg2 = 0x00, .CtrlReg3 = 0x00, .CtrlReg4 = 0x0C, .CtrlReg5 = 0x40};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LIS3MDL_Init(&handle, &config));

    ASSERT_INT_EQ(5, fake.transaction_count);

    static const uint8_t expected_regs[] = {LIS3MDL_REG_CTRL_REG1, LIS3MDL_REG_CTRL_REG2,
                                            LIS3MDL_REG_CTRL_REG3, LIS3MDL_REG_CTRL_REG4,
                                            LIS3MDL_REG_CTRL_REG5};
    const uint8_t expected_values[] = {config.CtrlReg1, config.CtrlReg2, config.CtrlReg3,
                                       config.CtrlReg4, config.CtrlReg5};

    for (size_t i = 0; i < 5; i++) {
        const FakeBusTransaction *write = fake_bus_transaction(&fake, i);
        ASSERT_TRUE(write != NULL);
        ASSERT_INT_EQ(FAKE_BUS_WRITE, write->op);
        ASSERT_INT_EQ(expected_regs[i], write->reg);
        ASSERT_INT_EQ(expected_values[i], write->data[0]);
    }
}

TEST(init_stops_at_the_first_failing_write) {
    for (size_t failing = 1; failing <= 5; failing++) {
        LIS3MDL_Handle_t handle;
        FakeBus fake;
        setup(&handle, &fake);
        fake_bus_fail_on_call(&fake, failing, LIBDRIVERS_ERR_TIMEOUT);

        const LIS3MDL_Config_t config = {.CtrlReg1 = 0x70,
                                         .CtrlReg2 = 0x01,
                                         .CtrlReg3 = 0x02,
                                         .CtrlReg4 = 0x0C,
                                         .CtrlReg5 = 0x40};
        ASSERT_STATUS_EQ(LIBDRIVERS_ERR_TIMEOUT, LIS3MDL_Init(&handle, &config));
        ASSERT_INT_EQ(failing, fake.transaction_count);
    }
}

TEST(read_reg_sets_the_auto_increment_bit) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    uint8_t buffer[6] = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK,
                     LIS3MDL_ReadReg(&handle, LIS3MDL_REG_OUT_X_L, buffer, sizeof(buffer)));

    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LIS3MDL_REG_OUT_X_L | LIS3MDL_AUTO_INCREMENT_BIT, read->reg);
}

TEST(read_raw_decodes_little_endian_axes) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // Distinct values per axis, so a swapped or duplicated axis is visible.
    seed_axes(&fake, LIS3MDL_REG_OUT_X_L, 1000, -2000, 3000);

    LIS3MDL_AxisData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LIS3MDL_ReadRaw(&handle, &data));

    ASSERT_INT_EQ(1000, data.X);
    ASSERT_INT_EQ(-2000, data.Y);
    ASSERT_INT_EQ(3000, data.Z);
}

TEST(read_raw_covers_the_full_signed_range) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // The extremes of int16_t, where a decode that forgets to sign-extend
    // turns -32768 into +32768 and overflows.
    seed_axes(&fake, LIS3MDL_REG_OUT_X_L, INT16_MIN, INT16_MAX, -1);

    LIS3MDL_AxisData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LIS3MDL_ReadRaw(&handle, &data));

    ASSERT_INT_EQ(INT16_MIN, data.X);
    ASSERT_INT_EQ(INT16_MAX, data.Y);
    ASSERT_INT_EQ(-1, data.Z);
}

TEST(read_raw_reads_six_bytes_from_out_x_l) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    seed_axes(&fake, LIS3MDL_REG_OUT_X_L, 1, 2, 3);

    LIS3MDL_AxisData_t data = {0};
    LIS3MDL_ReadRaw(&handle, &data);

    ASSERT_INT_EQ(1, fake.transaction_count);
    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LIS3MDL_REG_OUT_X_L | LIS3MDL_AUTO_INCREMENT_BIT, read->reg);
    ASSERT_INT_EQ(6, read->len);
}

TEST(read_hard_iron_offset_reads_its_own_register_block) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    // Seed the offsets and the axis outputs differently: a driver that read
    // the wrong base register would return the other block's values.
    seed_axes(&fake, LIS3MDL_REG_OFFSET_X_L_M, -100, 200, -300);
    seed_axes(&fake, LIS3MDL_REG_OUT_X_L, 1000, 2000, 3000);

    int16_t offsets[3] = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LIS3MDL_ReadHardIronOffset(&handle, offsets));

    ASSERT_INT_EQ(-100, offsets[0]);
    ASSERT_INT_EQ(200, offsets[1]);
    ASSERT_INT_EQ(-300, offsets[2]);

    const FakeBusTransaction *read = fake_bus_transaction(&fake, 0);
    ASSERT_TRUE(read != NULL);
    ASSERT_INT_EQ(LIS3MDL_REG_OFFSET_X_L_M | LIS3MDL_AUTO_INCREMENT_BIT, read->reg);
    ASSERT_INT_EQ(6, read->len);
}

TEST(readers_propagate_transport_failures) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);
    fake_bus_fail_on_call(&fake, 1, LIBDRIVERS_ERR_BUS);

    LIS3MDL_AxisData_t data = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, LIS3MDL_ReadRaw(&handle, &data));

    LIS3MDL_Handle_t offset_handle;
    FakeBus offset_fake;
    setup(&offset_handle, &offset_fake);
    fake_bus_fail_on_call(&offset_fake, 1, LIBDRIVERS_ERR_BUS);

    int16_t offsets[3] = {0};
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_BUS, LIS3MDL_ReadHardIronOffset(&offset_handle, offsets));
}

TEST(check_who_am_i_matches_the_datasheet_id) {
    LIS3MDL_Handle_t handle;
    FakeBus fake;
    setup(&handle, &fake);

    fake.regs[LIS3MDL_REG_WHO_AM_I] = LIS3MDL_WHO_AM_I_VALUE;
    ASSERT_STATUS_EQ(LIBDRIVERS_OK, LIS3MDL_CheckWhoAmI(&handle));

    fake.regs[LIS3MDL_REG_WHO_AM_I] = 0x00;
    ASSERT_STATUS_EQ(LIBDRIVERS_ERR_ID, LIS3MDL_CheckWhoAmI(&handle));
}

static const LibdriversTest tests[] = {
    LIBDRIVERS_TEST(init_writes_the_five_control_registers_in_order),
    LIBDRIVERS_TEST(init_stops_at_the_first_failing_write),
    LIBDRIVERS_TEST(read_reg_sets_the_auto_increment_bit),
    LIBDRIVERS_TEST(read_raw_decodes_little_endian_axes),
    LIBDRIVERS_TEST(read_raw_covers_the_full_signed_range),
    LIBDRIVERS_TEST(read_raw_reads_six_bytes_from_out_x_l),
    LIBDRIVERS_TEST(read_hard_iron_offset_reads_its_own_register_block),
    LIBDRIVERS_TEST(readers_propagate_transport_failures),
    LIBDRIVERS_TEST(check_who_am_i_matches_the_datasheet_id),
};
LIBDRIVERS_TEST_MAIN("lis3mdl", tests)
