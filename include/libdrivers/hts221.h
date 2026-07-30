#ifndef HTS221_H
#define HTS221_H

#include "libdrivers/bus.h"
#include <stdint.h>

/**
 * @file hts221.h
 * @brief Driver for the ST HTS221 humidity + temperature sensor (register bus).
 *
 * Speaks to the device through a Libdrivers_Bus_t transport, so the driver is
 * HAL-free. Temperature and humidity are computed from raw ADC counts using
 * the factory calibration; call HTS221_ReadCalibration() once after init
 * before the calibrated float readers will return meaningful values.
 */

// Register addresses
#define HTS221_REG_WHO_AM_I       0x0F

#define HTS221_REG_AV_CONF        0x10

#define HTS221_REG_CTRL_REG1      0x20
#define HTS221_REG_CTRL_REG2      0x21
#define HTS221_REG_CTRL_REG3      0x22

#define HTS221_REG_STATUS_REG     0x27

#define HTS221_REG_HUMIDITY_OUT_L 0x28
#define HTS221_REG_HUMIDITY_OUT_H 0x29
#define HTS221_REG_TEMP_OUT_L     0x2A
#define HTS221_REG_TEMP_OUT_H     0x2B

// Calibration register addresses (0x30 - 0x3F)
#define HTS221_REG_H0_RH_X2   0x30
#define HTS221_REG_H1_RH_X2   0x31
#define HTS221_REG_T0_DEGC_X8 0x32
#define HTS221_REG_T1_DEGC_X8 0x33
// 0x34 reserved
#define HTS221_REG_T0_T1_MSB   0x35
#define HTS221_REG_H0_T0_OUT_L 0x36
#define HTS221_REG_H0_T0_OUT_H 0x37
// 0x38, 0x39 reserved
#define HTS221_REG_H1_T0_OUT_L 0x3A
#define HTS221_REG_H1_T0_OUT_H 0x3B
#define HTS221_REG_T0_OUT_L    0x3C
#define HTS221_REG_T0_OUT_H    0x3D
#define HTS221_REG_T1_OUT_L    0x3E
#define HTS221_REG_T1_OUT_H    0x3F

// Bit mask values
#define HTS221_T0_MSB_MASK  0x03
#define HTS221_T1_MSB_MASK  0x0C
#define HTS221_T1_MSB_SHIFT 2

// Expected return value from WHO_AM_I
#define HTS221_WHO_AM_I_VALUE 0xBC

// Auto-increment bit (bit 7) - OR with register address for multi-byte reads
#define HTS221_AUTO_INCREMENT_BIT 0x80

/**
 * @brief HTS221 device handle.
 *
 * Holds the transport by value plus the factory calibration coefficients and
 * the most recent raw samples. Initialize @c bus (via a port) before use, and
 * populate the calibration fields with HTS221_ReadCalibration().
 */
typedef struct {
    Libdrivers_Bus_t bus; /**< Register-bus transport for this device. */
    uint16_t T0_degC_x8;  /**< Temp calibration point T0, in units of 1/8 degC. */
    uint16_t T1_degC_x8;  /**< Temp calibration point T1, in units of 1/8 degC. */
    uint8_t H0_rH_x2;     /**< Humidity calibration point H0, in units of 1/2 %RH. */
    uint8_t H1_rH_x2;     /**< Humidity calibration point H1, in units of 1/2 %RH. */
    int16_t H0_T0_OUT;    /**< Raw humidity ADC at calibration point H0. */
    int16_t H1_T0_OUT;    /**< Raw humidity ADC at calibration point H1. */
    int16_t T0_OUT;       /**< Raw temperature ADC at calibration point T0. */
    int16_t T1_OUT;       /**< Raw temperature ADC at calibration point T1. */
    int16_t H_OUT;        /**< Most recent raw humidity sample. */
    int16_t T_OUT;        /**< Most recent raw temperature sample. */
} HTS221_Handle_t;

/**
 * @brief Values for the three control registers, written by HTS221_Init().
 */
typedef struct {
    uint8_t CtrlReg1; /**< CTRL_REG1 (0x20). */
    uint8_t CtrlReg2; /**< CTRL_REG2 (0x21). */
    uint8_t CtrlReg3; /**< CTRL_REG3 (0x22). */
} HTS221_Config_t;

/**
 * @brief Configure the device by writing CTRL_REG1..3 from @p pConfig.
 *
 * @param pHandle Handle with an initialized bus.
 * @param pConfig Control-register values to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t HTS221_Init(HTS221_Handle_t *pHandle, const HTS221_Config_t *pConfig);

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
Libdrivers_Status_t HTS221_ReadReg(HTS221_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                   uint16_t Length);

/**
 * @brief Write a single byte to @p RegAddress.
 *
 * @param pHandle    Handle with an initialized bus.
 * @param RegAddress Register to write.
 * @param Value      Byte to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t HTS221_WriteReg(HTS221_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value);

/**
 * @brief Read the factory calibration coefficients into the handle.
 *
 * Must be called once after init; the calibrated float readers depend on the
 * coefficients it stores.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t HTS221_ReadCalibration(HTS221_Handle_t *pHandle);

/**
 * @brief Read the live raw humidity and temperature ADC counts into the handle.
 *
 * Updates HTS221_Handle_t::H_OUT and HTS221_Handle_t::T_OUT.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t HTS221_ReadRawOutput(HTS221_Handle_t *pHandle);

/**
 * @brief Verify the WHO_AM_I register matches HTS221_WHO_AM_I_VALUE.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK if the ID matches; LIBDRIVERS_ERR_ID on mismatch;
 *         otherwise a transport error.
 */
Libdrivers_Status_t HTS221_CheckWhoAmI(HTS221_Handle_t *pHandle);

/**
 * @brief Read a calibrated temperature in degrees Celsius.
 *
 * Takes a fresh raw sample and applies the stored calibration. Requires a
 * prior HTS221_ReadCalibration().
 *
 * @param pHandle            Handle with an initialized bus and loaded calibration.
 * @param[out] pTemperature  Written with the temperature in degrees C on
 *                           success; left unchanged on failure.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t HTS221_ReadTemperature(HTS221_Handle_t *pHandle, float *pTemperature);

/**
 * @brief Read a calibrated relative humidity in %RH.
 *
 * Takes a fresh raw sample and applies the stored calibration. Requires a
 * prior HTS221_ReadCalibration().
 *
 * @param pHandle         Handle with an initialized bus and loaded calibration.
 * @param[out] pHumidity  Written with the relative humidity in %RH on success;
 *                        left unchanged on failure.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t HTS221_ReadHumidity(HTS221_Handle_t *pHandle, float *pHumidity);

#endif // HTS221_H
