#include "lis3mdl.h"
#include <stdint.h>

HAL_StatusTypeDef LIS3MDL_ReadReg(LIS3MDL_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                  uint16_t Length) {

    // Set bit 7 (auto-increment bit) to enable multi-byte reads
    uint8_t ActualAddress = RegAddress | LIS3MDL_AUTO_INCREMENT_BIT;

    // Call Mem_Read from the STM32 HAL library
    // TODO: Need to make this vendor agnostic but for now it works for what I need
    return HAL_I2C_Mem_Read(pHandle->hi2c, LIS3MDL_I2C_ADDR, ActualAddress, I2C_MEMADD_SIZE_8BIT,
                            pBuffer, Length, HAL_MAX_DELAY);
}

bool LIS3MDL_CheckWhoAmI(LIS3MDL_Handle_t *pHandle) {

    // Byte to write the data from the WHO_AM_I register to
    uint8_t WhoAmIByte;

    // Read the WHO_AM_I register (0x0F)
    HAL_StatusTypeDef status = LIS3MDL_ReadReg(pHandle, LIS3MDL_REG_WHO_AM_I, &WhoAmIByte, 1);
    if (status != HAL_OK) {
        return false;
    }

    return WhoAmIByte == LIS3MDL_WHO_AM_I_VALUE;
}

HAL_StatusTypeDef LIS3MDL_ReadHardIronOffset(LIS3MDL_Handle_t *pHandle, int16_t *pOffsetXYZ) {

    // Buffer to write the XYZ bytes into
    uint8_t buffer[6];

    HAL_StatusTypeDef status = LIS3MDL_ReadReg(pHandle, LIS3MDL_REG_OFFSET_X_L_M, buffer, 6);
    if (status != HAL_OK) {
        return status;
    }

    pOffsetXYZ[0] = (int16_t)((buffer[1] << 8) | buffer[0]); // X
    pOffsetXYZ[1] = (int16_t)((buffer[3] << 8) | buffer[2]); // Y
    pOffsetXYZ[2] = (int16_t)((buffer[5] << 8) | buffer[4]); // Z

    return HAL_OK;
}
