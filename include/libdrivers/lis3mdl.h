#ifndef LIS3MDL_H
#define LIS3MDL_H

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_def.h"
#include <stdbool.h>
#include <stdint.h>

// LIS3MDL I2C Address (0x1E << 1)
#define LIS3MDL_I2C_ADDR 0x3C

// Register addresses
#define LIS3MDL_REG_OFFSET_X_L_M 0x05
#define LIS3MDL_REG_OFFSET_X_H_M 0x06
#define LIS3MDL_REG_OFFSET_Y_L_M 0x07
#define LIS3MDL_REG_OFFSET_Y_H_M 0x08
#define LIS3MDL_REG_OFFSET_Z_L_M 0x09
#define LIS3MDL_REG_OFFSET_Z_H_M 0x0A

#define LIS3MDL_REG_WHO_AM_I     0x0F

#define LIS3MDL_REG_CTRL_REG1    0x20
#define LIS3MDL_REG_CTRL_REG2    0x21
#define LIS3MDL_REG_CTRL_REG3    0x22
#define LIS3MDL_REG_CTRL_REG4    0x23
#define LIS3MDL_REG_CTRL_REG5    0x24

#define LIS3MDL_REG_STATUS_REG   0x27

#define LIS3MDL_REG_OUT_X_L      0x28
#define LIS3MDL_REG_OUT_X_H      0x29
#define LIS3MDL_REG_OUT_Y_L      0x2A
#define LIS3MDL_REG_OUT_Y_H      0x2B
#define LIS3MDL_REG_OUT_Z_L      0x2C
#define LIS3MDL_REG_OUT_Z_H      0x2D

#define LIS3MDL_REG_TEMP_OUT_L   0x2E
#define LIS3MDL_REG_TEMP_OUT_H   0x2F

#define LIS3MDL_REG_INT_CFG      0x30
#define LIS3MDL_REG_INT_SRC      0x31

#define LIS3MDL_REG_INT_THS_L    0x32
#define LIS3MDL_REG_INT_THS_H    0x33

// Expected return values
#define LIS3MDL_WHO_AM_I_VALUE 0x3D

// Auto-increment bit (bit 7) - OR with register address for multi-byte reads
#define LIS3MDL_AUTO_INCREMENT_BIT 0x80

typedef struct {
    I2C_HandleTypeDef *hi2c;
} LIS3MDL_Handle_t;

// Struct to hold the bytes for each CTRL_REG
typedef struct {
    uint8_t CtrlReg1;
    uint8_t CtrlReg2;
    uint8_t CtrlReg3;
    uint8_t CtrlReg4;
    uint8_t CtrlReg5;
} LIS3MDL_Config_t;

// Initialization function
HAL_StatusTypeDef LIS3MDL_Init(LIS3MDL_Handle_t *pHandle, const LIS3MDL_Config_t *pConfig);

// Function to read a register given a register address
HAL_StatusTypeDef LIS3MDL_ReadReg(LIS3MDL_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                  uint16_t Length);

// Function to write to a register given a register address
HAL_StatusTypeDef LIS3MDL_WriteReg(LIS3MDL_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value);

// Function to read the hard-iron offset registers
HAL_StatusTypeDef LIS3MDL_ReadHardIronOffset(LIS3MDL_Handle_t *pHandle, int16_t *pOffsetXYZ);

// Function to verify the WHO_AM_I register returns the correct data
bool LIS3MDL_CheckWhoAmI(LIS3MDL_Handle_t *pHandle);

#endif // LIS3MDL_H
