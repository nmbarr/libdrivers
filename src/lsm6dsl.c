#include "libdrivers/lsm6dsl.h"
#include "libdrivers/bus.h"
#include <stdint.h>

// Accelerometer sensitivity in mg/LSB x1000, indexed by LSM6DSL_XLFullScale_t
// (2g, 16g, 4g, 8g -- the FS_XL raw code order, not ascending g).
static const int32_t LSM6DSL_XLFullScale_Sensitivity[4] = {61, 488, 122, 244};

// Gyroscope sensitivity in mdps/LSB x8, indexed by LSM6DSL_GyroFullScale_t
// (250, 500, 1000, 2000, 125 dps). x8 clears the .375/.5 fractions in the
// datasheet's 4.375/8.75/17.5 mdps/LSB values exactly, with no rounding loss.
static const int32_t LSM6DSL_GyroFullScale_Sensitivity[5] = {70, 140, 280, 560, 35};

Libdrivers_Status_t LSM6DSL_Init(LSM6DSL_Handle_t *pHandle, const LSM6DSL_Config_t *pConfig) {

    // Status from each register write
    Libdrivers_Status_t status;

    // Bring up both sub-sensors and common config: accel (CTRL1_XL),
    // gyro (CTRL2_G), then CTRL3_C. Stop at the first failing write.
    // CTRL3_C keeps IF_INC set (auto-increment), so leave that bit alone here.
    status = LSM6DSL_WriteReg(pHandle, LSM6DSL_REG_CTRL1_XL, pConfig->CtrlReg1_XL);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = LSM6DSL_WriteReg(pHandle, LSM6DSL_REG_CTRL2_G, pConfig->CtrlReg2_G);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = LSM6DSL_WriteReg(pHandle, LSM6DSL_REG_CTRL3_C, pConfig->CtrlReg3_C);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Cache the configured full-scale ranges so the _mg/_mdps readers can scale
    // without re-reading CTRL1_XL/CTRL2_G. FS_XL[1:0] sits at bits [3:2]; its
    // raw code already matches LSM6DSL_XLFullScale_t's values once shifted down.
    pHandle->XLFullScale =
        (LSM6DSL_XLFullScale_t)((pConfig->CtrlReg1_XL & LSM6DSL_FS_XL_BIT_MASK) >>
                                LSM6DSL_FS_XL_BIT_SHIFT);

    // FS_125 (bit 1) overrides FS_G[1:0] when set, so it must be checked first;
    // otherwise decode FS_G[1:0] the same way as FS_XL above.
    if (pConfig->CtrlReg2_G & LSM6DSL_FS_125_BIT_MASK) {
        pHandle->GyroFullScale = LSM6DSL_FS_G_125;
    } else {
        pHandle->GyroFullScale =
            (LSM6DSL_GyroFullScale_t)((pConfig->CtrlReg2_G & LSM6DSL_FS_G_BIT_MASK) >>
                                      LSM6DSL_FS_G_BIT_SHIFT);
    }

    return LIBDRIVERS_OK;
}

// Send the plain address: the device auto-increments internally (IF_INC), so
// multi-byte reads walk consecutive registers without setting any address bit.
Libdrivers_Status_t LSM6DSL_ReadReg(LSM6DSL_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                    uint16_t Length) {
    return pHandle->bus.read(pHandle->bus.ctx, RegAddress, pBuffer, Length);
}

Libdrivers_Status_t LSM6DSL_WriteReg(LSM6DSL_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value) {
    return pHandle->bus.write(pHandle->bus.ctx, RegAddress, &Value, 1);
}

Libdrivers_Status_t LSM6DSL_ReadRawXL(LSM6DSL_Handle_t *pHandle, LSM6DSL_XLData_t *pData) {

    // Six accelerometer output bytes, X/Y/Z as little-endian L,H pairs
    uint8_t buffer[6];

    // One burst from OUTX_L_XL grabs all six; auto-increment walks the rest
    Libdrivers_Status_t status =
        LSM6DSL_ReadReg(pHandle, LSM6DSL_REG_OUTX_L_XL, buffer, sizeof(buffer));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Combine each L,H pair; the int16_t cast makes negative readings sign-extend
    pData->X = (int16_t)((buffer[1] << 8) | buffer[0]); // X
    pData->Y = (int16_t)((buffer[3] << 8) | buffer[2]); // Y
    pData->Z = (int16_t)((buffer[5] << 8) | buffer[4]); // Z

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t LSM6DSL_ReadRawGyro(LSM6DSL_Handle_t *pHandle, LSM6DSL_GyroData_t *pData) {

    // Six gyroscope output bytes, X/Y/Z as little-endian L,H pairs
    uint8_t buffer[6];

    // One burst from OUTX_L_G grabs all six; auto-increment walks the rest
    Libdrivers_Status_t status =
        LSM6DSL_ReadReg(pHandle, LSM6DSL_REG_OUTX_L_G, buffer, sizeof(buffer));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Combine each L,H pair; the int16_t cast makes negative readings sign-extend
    pData->X = (int16_t)((buffer[1] << 8) | buffer[0]); // X
    pData->Y = (int16_t)((buffer[3] << 8) | buffer[2]); // Y
    pData->Z = (int16_t)((buffer[5] << 8) | buffer[4]); // Z

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t LSM6DSL_ReadXL_mg(LSM6DSL_Handle_t *pHandle, LSM6DSL_XLData_mg_t *pData) {

    // Take a fresh raw sample; nothing to scale on failure
    LSM6DSL_XLData_t RawXLData;

    Libdrivers_Status_t status = LSM6DSL_ReadRawXL(pHandle, &RawXLData);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // FS decoded once by LSM6DSL_Init(); no bus access needed here
    int32_t sensitivity = LSM6DSL_XLFullScale_Sensitivity[pHandle->XLFullScale];

    // Table is mg/LSB x1000, so undo the x1000 after multiplying
    pData->X_mg = (int32_t)RawXLData.X * sensitivity / 1000;
    pData->Y_mg = (int32_t)RawXLData.Y * sensitivity / 1000;
    pData->Z_mg = (int32_t)RawXLData.Z * sensitivity / 1000;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t LSM6DSL_ReadGyro_mdps(LSM6DSL_Handle_t *pHandle,
                                          LSM6DSL_GyroData_mdps_t *pData) {

    // Take a fresh raw sample; nothing to scale on failure
    LSM6DSL_GyroData_t RawGyroData;

    Libdrivers_Status_t status = LSM6DSL_ReadRawGyro(pHandle, &RawGyroData);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // FS decoded once by LSM6DSL_Init(); no bus access needed here
    int32_t sensitivity = LSM6DSL_GyroFullScale_Sensitivity[pHandle->GyroFullScale];

    // Table is mdps/LSB x8, so undo the x8 after multiplying
    pData->X_mdps = (int32_t)RawGyroData.X * sensitivity / 8;
    pData->Y_mdps = (int32_t)RawGyroData.Y * sensitivity / 8;
    pData->Z_mdps = (int32_t)RawGyroData.Z * sensitivity / 8;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t LSM6DSL_CheckWhoAmI(LSM6DSL_Handle_t *pHandle) {
    return Libdrivers_Bus_CheckWhoAmI(&pHandle->bus, LSM6DSL_REG_WHO_AM_I, LSM6DSL_WHO_AM_I_VALUE);
}
