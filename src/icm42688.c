#include "libdrivers/icm42688.h"
#include "libdrivers/bus.h"
#include <stddef.h>
#include <stdint.h>

// Accelerometer sensitivity in LSB/g, indexed by ICM42688_AccelFullScale_t
// (16g, 8g, 4g, 2g -- the ACCEL_FS_SEL code order, not ascending g).
static const int32_t ICM42688_AccelFullScale_Sensitivity[4] = {2048, 4096, 8192, 16384};

// Gyroscope sensitivity in LSB/dps x10, indexed by ICM42688_GyroFullScale_t
// (2000..15.625 dps). x10 clears the .4/.5/.3/.6 fractions in the datasheet's
// LSB/dps values exactly, with no rounding loss.
static const int32_t ICM42688_GyroFullScale_Sensitivity[8] = {164,  328,  655,   1310,
                                                              2620, 5243, 10486, 20972};

// Temperature conversion (datasheet): degC = (raw / COUNTS_PER_DEGC) + OFFSET_DEGC
static const float ICM42688_TEMPERATURE_COUNTS_PER_DEGC = 132.48f;
static const float ICM42688_TEMPERATURE_OFFSET_DEGC = 25.0f;

Libdrivers_Status_t ICM42688_Init(ICM42688_Handle_t *pHandle, const ICM42688_Config_t *pConfig) {

    Libdrivers_Status_t status;

    status = ICM42688_WriteReg(pHandle, ICM42688_REG_GYRO_CONFIG0, pConfig->GyroConfig0);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = ICM42688_WriteReg(pHandle, ICM42688_REG_ACCEL_CONFIG0, pConfig->AccelConfig0);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = ICM42688_WriteReg(pHandle, ICM42688_REG_PWR_MGMT0, pConfig->PwrMgmt0);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // OFF->mode transition needs a 200us settle before further register access
    // (PWR_MGMT0 spec); round up to the delay hook's 1ms granularity. The delay
    // is mandatory here, so a missing hook is an error, not a silent skip.
    if (pHandle->bus.delay == NULL) {
        return LIBDRIVERS_ERR_ARG;
    }
    pHandle->bus.delay(pHandle->bus.ctx, 1);

    // Cache the configured full-scale ranges so the _mg/_mdps readers can scale
    // without re-reading the config registers. The raw FS_SEL code, once masked
    // and shifted down, already matches the *FullScale_t enum values.
    pHandle->AccelFullScale =
        (ICM42688_AccelFullScale_t)((pConfig->AccelConfig0 & ICM42688_ACCEL_FS_SEL_BIT_MASK) >>
                                    ICM42688_ACCEL_FS_SEL_BIT_SHIFT);
    pHandle->GyroFullScale =
        (ICM42688_GyroFullScale_t)((pConfig->GyroConfig0 & ICM42688_GYRO_FS_SEL_BIT_MASK) >>
                                   ICM42688_GYRO_FS_SEL_BIT_SHIFT);

    return LIBDRIVERS_OK;
}

// Send the plain address: the device auto-increments internally on burst reads,
// so multi-byte reads walk consecutive registers without setting any address bit.
Libdrivers_Status_t ICM42688_ReadReg(ICM42688_Handle_t *pHandle, uint8_t RegAddress,
                                     uint8_t *pBuffer, uint16_t Length) {
    return pHandle->bus.read(pHandle->bus.ctx, RegAddress, pBuffer, Length);
}

Libdrivers_Status_t ICM42688_WriteReg(ICM42688_Handle_t *pHandle, uint8_t RegAddress,
                                      uint8_t Value) {
    return pHandle->bus.write(pHandle->bus.ctx, RegAddress, &Value, 1);
}

Libdrivers_Status_t ICM42688_ReadRawAccel(ICM42688_Handle_t *pHandle, ICM42688_AccelData_t *pData) {

    // Six accelerometer output bytes, X/Y/Z as big-endian H,L pairs
    uint8_t buffer[6];

    // One burst from ACCEL_DATA_X1 grabs all six; auto-increment walks the rest
    Libdrivers_Status_t status =
        ICM42688_ReadReg(pHandle, ICM42688_REG_ACCEL_DATA_X1, buffer, sizeof(buffer));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Combine each H,L pair (high byte first); the int16_t cast sign-extends
    pData->X = (int16_t)((buffer[0] << 8) | buffer[1]); // X
    pData->Y = (int16_t)((buffer[2] << 8) | buffer[3]); // Y
    pData->Z = (int16_t)((buffer[4] << 8) | buffer[5]); // Z

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t ICM42688_ReadRawGyro(ICM42688_Handle_t *pHandle, ICM42688_GyroData_t *pData) {

    // Six gyroscope output bytes, X/Y/Z as big-endian H,L pairs
    uint8_t buffer[6];

    // One burst from GYRO_DATA_X1 grabs all six; auto-increment walks the rest
    Libdrivers_Status_t status =
        ICM42688_ReadReg(pHandle, ICM42688_REG_GYRO_DATA_X1, buffer, sizeof(buffer));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Combine each H,L pair (high byte first); the int16_t cast sign-extends
    pData->X = (int16_t)((buffer[0] << 8) | buffer[1]); // X
    pData->Y = (int16_t)((buffer[2] << 8) | buffer[3]); // Y
    pData->Z = (int16_t)((buffer[4] << 8) | buffer[5]); // Z

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t ICM42688_ReadAccel_mg(ICM42688_Handle_t *pHandle,
                                          ICM42688_AccelData_mg_t *pData) {

    // Take a fresh raw sample; nothing to scale on failure
    ICM42688_AccelData_t RawAccelData;

    Libdrivers_Status_t status = ICM42688_ReadRawAccel(pHandle, &RawAccelData);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // FS decoded once by ICM42688_Init(); no bus access needed here
    int32_t sensitivity = ICM42688_AccelFullScale_Sensitivity[pHandle->AccelFullScale];

    // Table is LSB/g, so mg = raw * 1000 / (LSB/g); the x1000 turns g into mg
    pData->X_mg = (int32_t)RawAccelData.X * 1000 / sensitivity;
    pData->Y_mg = (int32_t)RawAccelData.Y * 1000 / sensitivity;
    pData->Z_mg = (int32_t)RawAccelData.Z * 1000 / sensitivity;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t ICM42688_ReadGyro_mdps(ICM42688_Handle_t *pHandle,
                                           ICM42688_GyroData_mdps_t *pData) {

    // Take a fresh raw sample; nothing to scale on failure
    ICM42688_GyroData_t RawGyroData;

    Libdrivers_Status_t status = ICM42688_ReadRawGyro(pHandle, &RawGyroData);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // FS decoded once by ICM42688_Init(); no bus access needed here
    int32_t sensitivity = ICM42688_GyroFullScale_Sensitivity[pHandle->GyroFullScale];

    // Table is LSB/dps x10, so mdps = raw * 10000 / (LSB/dps x10): 1000 turns dps
    // into mdps, the extra x10 cancels the table's x10.
    pData->X_mdps = (int32_t)RawGyroData.X * 10000 / sensitivity;
    pData->Y_mdps = (int32_t)RawGyroData.Y * 10000 / sensitivity;
    pData->Z_mdps = (int32_t)RawGyroData.Z * 10000 / sensitivity;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t ICM42688_ReadTemperature(ICM42688_Handle_t *pHandle, float *pTemperature) {

    // Two temperature output bytes, big-endian H,L (TEMP_DATA1 then TEMP_DATA0)
    uint8_t buffer[2];

    Libdrivers_Status_t status =
        ICM42688_ReadReg(pHandle, ICM42688_REG_TEMP_DATA1, buffer, sizeof(buffer));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Combine the H,L pair; the int16_t cast sign-extends a negative reading
    int16_t raw = (int16_t)((buffer[0] << 8) | buffer[1]);

    // Datasheet: degC = (raw / 132.48) + 25
    *pTemperature = raw / ICM42688_TEMPERATURE_COUNTS_PER_DEGC + ICM42688_TEMPERATURE_OFFSET_DEGC;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t ICM42688_CheckWhoAmI(ICM42688_Handle_t *pHandle) {
    return Libdrivers_Bus_CheckWhoAmI(&pHandle->bus, ICM42688_REG_WHO_AM_I,
                                      ICM42688_WHO_AM_I_VALUE);
}
