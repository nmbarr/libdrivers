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
