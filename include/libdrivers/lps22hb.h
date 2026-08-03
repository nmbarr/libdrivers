#ifndef LPS22HB_H
#define LPS22HB_H

#include "libdrivers/bus.h"
#include <stdint.h>

/**
 * @file lps22hb.h
 * @brief Driver for the ST LPS22HB pressure + temperature sensor (register bus).
 *
 * Speaks to the device through a Libdrivers_Bus_t transport, so the driver is
 * HAL-free. Pressure and temperature are computed directly from raw ADC
 * counts; the device needs no factory calibration. PRESS_OUT and TEMP_OUT
 * are contiguous registers, so a single auto-increment read fills both.
 */

// Register addresses
#define LPS22HB_REG_INTERRUPT_CFG 0x0B
#define LPS22HB_REG_THS_P_L       0x0C
#define LPS22HB_REG_THS_P_H       0x0D

#define LPS22HB_REG_WHO_AM_I      0x0F

#define LPS22HB_REG_CTRL_REG1     0x10
#define LPS22HB_REG_CTRL_REG2     0x11
#define LPS22HB_REG_CTRL_REG3     0x12

#define LPS22HB_REG_FIFO_CTRL     0x14
#define LPS22HB_REG_REF_P_XL      0x15
#define LPS22HB_REG_REF_P_L       0x16
#define LPS22HB_REG_REF_P_H       0x17
#define LPS22HB_REG_RPDS_L        0x18
#define LPS22HB_REG_RPDS_H        0x19
#define LPS22HB_REG_RES_CONF      0x1A

#define LPS22HB_REG_INT_SOURCE    0x25
#define LPS22HB_REG_FIFO_STATUS   0x26
#define LPS22HB_REG_STATUS        0x27

#define LPS22HB_REG_PRESS_OUT_XL  0x28
#define LPS22HB_REG_PRESS_OUT_L   0x29
#define LPS22HB_REG_PRESS_OUT_H   0x2A
#define LPS22HB_REG_TEMP_OUT_L    0x2B
#define LPS22HB_REG_TEMP_OUT_H    0x2C

#define LPS22HB_REG_LPFP_RES      0x33

// Expected return value from WHO_AM_I
#define LPS22HB_WHO_AM_I_VALUE 0xB1

// Auto-increment bit (bit 7) - OR with register address for multi-byte reads
#define LPS22HB_AUTO_INCREMENT_BIT 0x80

/**
 * @brief LPS22HB device handle.
 *
 * Holds the transport by value plus the most recent raw pressure and
 * temperature samples. Initialize @c bus (via a port) before use.
 */
typedef struct {
    Libdrivers_Bus_t bus; /**< Register-bus transport for this device. */
    int32_t P_OUT;        /**< Most recent raw pressure sample (24-bit, sign-extended). */
    int16_t T_OUT;        /**< Most recent raw temperature sample. */
} LPS22HB_Handle_t;

/**
 * @brief Values for the three control registers, written by LPS22HB_Init().
 */
typedef struct {
    uint8_t CtrlReg1; /**< CTRL_REG1 (0x10). */
    uint8_t CtrlReg2; /**< CTRL_REG2 (0x11). */
    uint8_t CtrlReg3; /**< CTRL_REG3 (0x12). */
} LPS22HB_Config_t;

/**
 * @brief Configure the device by writing CTRL_REG1..3 from @p pConfig.
 *
 * @param pHandle Handle with an initialized bus.
 * @param pConfig Control-register values to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LPS22HB_Init(LPS22HB_Handle_t *pHandle, const LPS22HB_Config_t *pConfig);

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
Libdrivers_Status_t LPS22HB_ReadReg(LPS22HB_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                    uint16_t Length);

/**
 * @brief Write a single byte to @p RegAddress.
 *
 * @param pHandle    Handle with an initialized bus.
 * @param RegAddress Register to write.
 * @param Value      Byte to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LPS22HB_WriteReg(LPS22HB_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value);

/**
 * @brief Read the live raw pressure and temperature ADC counts into the handle.
 *
 * PRESS_OUT_XL..TEMP_OUT_H are contiguous, so this reads them in a single
 * auto-incrementing transfer. Updates LPS22HB_Handle_t::P_OUT and
 * LPS22HB_Handle_t::T_OUT.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LPS22HB_ReadRawOutput(LPS22HB_Handle_t *pHandle);

/**
 * @brief Verify the WHO_AM_I register matches LPS22HB_WHO_AM_I_VALUE.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK if the ID matches; LIBDRIVERS_ERR_ID on mismatch;
 *         otherwise a transport error.
 */
Libdrivers_Status_t LPS22HB_CheckWhoAmI(LPS22HB_Handle_t *pHandle);

/**
 * @brief Read a pressure in hPa.
 *
 * Takes a fresh raw sample and converts it (4096 LSB/hPa).
 *
 * @param pHandle         Handle with an initialized bus.
 * @param[out] pPressure  Written with the pressure in hPa on success; left
 *                        unchanged on failure.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LPS22HB_ReadPressure(LPS22HB_Handle_t *pHandle, float *pPressure);

/**
 * @brief Read a temperature in degrees Celsius.
 *
 * Takes a fresh raw sample and converts it (100 LSB/degC).
 *
 * @param pHandle            Handle with an initialized bus.
 * @param[out] pTemperature  Written with the temperature in degrees C on
 *                           success; left unchanged on failure.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LPS22HB_ReadTemperature(LPS22HB_Handle_t *pHandle, float *pTemperature);

#endif // LPS22HB_H
