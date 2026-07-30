#ifndef LIS3MDL_H
#define LIS3MDL_H

#include "libdrivers/bus.h"
#include <stdint.h>

/**
 * @file lis3mdl.h
 * @brief Driver for the ST LIS3MDL 3-axis magnetometer (register bus).
 *
 * Speaks to the device through a Libdrivers_Bus_t transport, so the driver
 * is HAL-free: a port supplies the actual I2C/SPI access. Register reads set
 * the auto-increment bit internally for multi-byte transfers.
 */

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

/**
 * @brief LIS3MDL device handle.
 *
 * Holds the transport by value; initialize @c bus (via a port) before
 * calling any driver function.
 */
typedef struct {
    Libdrivers_Bus_t bus; /**< Register-bus transport for this device. */
} LIS3MDL_Handle_t;

/**
 * @brief Values for the five control registers, written by LIS3MDL_Init().
 */
typedef struct {
    uint8_t CtrlReg1; /**< CTRL_REG1 (0x20). */
    uint8_t CtrlReg2; /**< CTRL_REG2 (0x21). */
    uint8_t CtrlReg3; /**< CTRL_REG3 (0x22). */
    uint8_t CtrlReg4; /**< CTRL_REG4 (0x23). */
    uint8_t CtrlReg5; /**< CTRL_REG5 (0x24). */
} LIS3MDL_Config_t;

/**
 * @brief Signed 16-bit per-axis sample.
 */
typedef struct {
    int16_t X; /**< X axis. */
    int16_t Y; /**< Y axis. */
    int16_t Z; /**< Z axis. */
} LIS3MDL_AxisData_t;

/**
 * @brief Configure the device by writing CTRL_REG1..5 from @p pConfig.
 *
 * Stops at the first failing write and returns its status.
 *
 * @param pHandle Handle with an initialized bus.
 * @param pConfig Control-register values to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LIS3MDL_Init(LIS3MDL_Handle_t *pHandle, const LIS3MDL_Config_t *pConfig);

/**
 * @brief Read @p Length bytes starting at @p RegAddress.
 *
 * Sets the auto-increment bit so multi-byte reads walk consecutive registers.
 *
 * @param pHandle       Handle with an initialized bus.
 * @param RegAddress    First register to read.
 * @param[out] pBuffer  Destination buffer; must hold @p Length bytes.
 * @param Length        Number of bytes to read.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LIS3MDL_ReadReg(LIS3MDL_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                    uint16_t Length);

/**
 * @brief Write a single byte to @p RegAddress.
 *
 * @param pHandle    Handle with an initialized bus.
 * @param RegAddress Register to write.
 * @param Value      Byte to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LIS3MDL_WriteReg(LIS3MDL_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value);

/**
 * @brief Read the three hard-iron offset registers.
 *
 * @param pHandle          Handle with an initialized bus.
 * @param[out] pOffsetXYZ  Array of at least 3 int16_t; filled with X, Y, Z.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LIS3MDL_ReadHardIronOffset(LIS3MDL_Handle_t *pHandle, int16_t *pOffsetXYZ);

/**
 * @brief Read the raw ADC counts from the X, Y, Z output registers.
 *
 * @param pHandle     Handle with an initialized bus.
 * @param[out] pData  Filled with the signed per-axis samples.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LIS3MDL_ReadRaw(LIS3MDL_Handle_t *pHandle, LIS3MDL_AxisData_t *pData);

/**
 * @brief Verify the WHO_AM_I register matches LIS3MDL_WHO_AM_I_VALUE.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK if the ID matches; LIBDRIVERS_ERR_ID on mismatch;
 *         otherwise a transport error.
 */
Libdrivers_Status_t LIS3MDL_CheckWhoAmI(LIS3MDL_Handle_t *pHandle);

#endif // LIS3MDL_H
