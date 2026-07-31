#include "libdrivers/lsm6dsl.h"
#include "libdrivers/bus.h"
#include <stdint.h>

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

Libdrivers_Status_t LSM6DSL_CheckWhoAmI(LSM6DSL_Handle_t *pHandle) {
    return Libdrivers_Bus_CheckWhoAmI(&pHandle->bus, LSM6DSL_REG_WHO_AM_I, LSM6DSL_WHO_AM_I_VALUE);
}
