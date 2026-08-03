#ifndef ICM42688_H
#define ICM42688_H

#include "libdrivers/bus.h"
#include <stdint.h>

/**
 * @file icm42688.h
 * @brief Driver for the InvenSense ICM-42688-P accelerometer + gyroscope (register bus).
 *
 * Speaks to the device through a Libdrivers_Bus_t transport, so the driver is
 * HAL-free: a port supplies the actual I2C/SPI access. Registers are split
 * across user banks 0-4, selected via REG_BANK_SEL (0x76); the device powers
 * up in bank 0. All basic operation -- identity, power control, ODR/full-scale
 * config, and accel/gyro/temperature output -- lives in bank 0, so this driver
 * stays there and never switches banks. Banks 1-4 (filter tuning, self-test,
 * APEX, FIFO watermark) are out of scope here.
 */

// Register addresses (User Bank 0)
#define ICM42688_REG_DEVICE_CONFIG      0x11
#define ICM42688_REG_DRIVE_CONFIG       0x13
#define ICM42688_REG_INT_CONFIG         0x14
#define ICM42688_REG_FIFO_CONFIG        0x16
#define ICM42688_REG_TEMP_DATA1         0x1D
#define ICM42688_REG_TEMP_DATA0         0x1E
#define ICM42688_REG_ACCEL_DATA_X1      0x1F
#define ICM42688_REG_ACCEL_DATA_X0      0x20
#define ICM42688_REG_ACCEL_DATA_Y1      0x21
#define ICM42688_REG_ACCEL_DATA_Y0      0x22
#define ICM42688_REG_ACCEL_DATA_Z1      0x23
#define ICM42688_REG_ACCEL_DATA_Z0      0x24
#define ICM42688_REG_GYRO_DATA_X1       0x25
#define ICM42688_REG_GYRO_DATA_X0       0x26
#define ICM42688_REG_GYRO_DATA_Y1       0x27
#define ICM42688_REG_GYRO_DATA_Y0       0x28
#define ICM42688_REG_GYRO_DATA_Z1       0x29
#define ICM42688_REG_GYRO_DATA_Z0       0x2A
#define ICM42688_REG_TMST_FSYNCH        0x2B
#define ICM42688_REG_TMST_FSYNCL        0x2C
#define ICM42688_REG_INT_STATUS         0x2D
#define ICM42688_REG_FIFO_COUNTH        0x2E
#define ICM42688_REG_FIFO_COUNTL        0x2F
#define ICM42688_REG_FIFO_DATA          0x30
#define ICM42688_REG_APEX_DATA0         0x31
#define ICM42688_REG_APEX_DATA1         0x32
#define ICM42688_REG_APEX_DATA2         0x33
#define ICM42688_REG_APEX_DATA3         0x34
#define ICM42688_REG_APEX_DATA4         0x35
#define ICM42688_REG_APEX_DATA5         0x36
#define ICM42688_REG_INT_STATUS2        0x37
#define ICM42688_REG_INT_STATUS3        0x38
#define ICM42688_REG_SIGNAL_PATH_RESET  0x4B
#define ICM42688_REG_INTF_CONFIG0       0x4C
#define ICM42688_REG_INTF_CONFIG1       0x4D
#define ICM42688_REG_PWR_MGMT0          0x4E
#define ICM42688_REG_GYRO_CONFIG0       0x4F
#define ICM42688_REG_ACCEL_CONFIG0      0x50
#define ICM42688_REG_GYRO_CONFIG1       0x51
#define ICM42688_REG_GYRO_ACCEL_CONFIG0 0x52
#define ICM42688_REG_ACCEL_CONFIG1      0x53
#define ICM42688_REG_TMST_CONFIG        0x54
#define ICM42688_REG_APEX_CONFIG0       0x56
#define ICM42688_REG_SMD_CONFIG         0x57
#define ICM42688_REG_FIFO_CONFIG1       0x5F
#define ICM42688_REG_FIFO_CONFIG2       0x60
#define ICM42688_REG_FIFO_CONFIG3       0x61
#define ICM42688_REG_FSYNC_CONFIG       0x62
#define ICM42688_REG_INT_CONFIG0        0x63
#define ICM42688_REG_INT_CONFIG1        0x64
#define ICM42688_REG_INT_SOURCE0        0x65
#define ICM42688_REG_INT_SOURCE1        0x66
#define ICM42688_REG_INT_SOURCE3        0x68
#define ICM42688_REG_INT_SOURCE4        0x69
#define ICM42688_REG_FIFO_LOST_PKT0     0x6C
#define ICM42688_REG_FIFO_LOST_PKT1     0x6D
#define ICM42688_REG_SELF_TEST_CONFIG   0x70
#define ICM42688_REG_WHO_AM_I           0x75
#define ICM42688_REG_BANK_SEL           0x76

// Bit mask values
#define ICM42688_ACCEL_FS_SEL_BIT_MASK 0xE0 /**< ACCEL_FS_SEL[2:0] field within ACCEL_CONFIG0. */
#define ICM42688_ACCEL_ODR_BIT_MASK    0x0F /**< ACCEL_ODR[3:0] field within ACCEL_CONFIG0. */
#define ICM42688_GYRO_FS_SEL_BIT_MASK  0xE0 /**< GYRO_FS_SEL[2:0] field within GYRO_CONFIG0. */
#define ICM42688_GYRO_ODR_BIT_MASK     0x0F /**< GYRO_ODR[3:0] field within GYRO_CONFIG0. */
#define ICM42688_GYRO_MODE_BIT_MASK    0x0C /**< GYRO_MODE[1:0] field within PWR_MGMT0. */
#define ICM42688_ACCEL_MODE_BIT_MASK   0x03 /**< ACCEL_MODE[1:0] field within PWR_MGMT0. */
#define ICM42688_BANK_SEL_BIT_MASK     0x07 /**< BANK_SEL[2:0] field within REG_BANK_SEL. */

// Bit shift values
#define ICM42688_ACCEL_FS_SEL_BIT_SHIFT                                                            \
    5 /**< Shift to right-align ACCEL_FS_SEL[2:0] after masking. */
#define ICM42688_GYRO_FS_SEL_BIT_SHIFT                                                             \
    5                                  /**< Shift to right-align GYRO_FS_SEL[2:0] after masking. */
#define ICM42688_GYRO_MODE_BIT_SHIFT 2 /**< Shift to right-align GYRO_MODE[1:0] after masking. */

// Expected WHO_AM_I value
#define ICM42688_WHO_AM_I_VALUE 0x47

/**
 * @brief Accelerometer full-scale range (ACCEL_FS_SEL[2:0] in ACCEL_CONFIG0).
 *
 * Enumerator values equal the raw 3-bit ACCEL_FS_SEL code, so decoding is a
 * direct cast of the masked, shifted register value. Note the descending g
 * order: code 000 is the widest range (+/-16 g), not the narrowest.
 */
typedef enum {
    ICM42688_ACCEL_FS_16G = 0x00, /**< ±16 g, 2048 LSB/g. */
    ICM42688_ACCEL_FS_8G = 0x01,  /**< ±8 g,  4096 LSB/g. */
    ICM42688_ACCEL_FS_4G = 0x02,  /**< ±4 g,  8192 LSB/g. */
    ICM42688_ACCEL_FS_2G = 0x03,  /**< ±2 g,  16384 LSB/g. */
} ICM42688_AccelFullScale_t;

/**
 * @brief Gyroscope full-scale range (GYRO_FS_SEL[2:0] in GYRO_CONFIG0).
 *
 * Enumerator values equal the raw 3-bit GYRO_FS_SEL code, so decoding is a
 * direct cast of the masked, shifted register value. Like the accelerometer,
 * code 000 is the widest range (+/-2000 dps).
 */
typedef enum {
    ICM42688_GYRO_FS_2000 = 0x00,   /**< ±2000 dps, 16.4 LSB/dps. */
    ICM42688_GYRO_FS_1000 = 0x01,   /**< ±1000 dps, 32.8 LSB/dps. */
    ICM42688_GYRO_FS_500 = 0x02,    /**< ±500 dps, 65.5 LSB/dps. */
    ICM42688_GYRO_FS_250 = 0x03,    /**< ±250 dps, 131 LSB/dps. */
    ICM42688_GYRO_FS_125 = 0x04,    /**< ±125 dps, 262 LSB/dps. */
    ICM42688_GYRO_FS_62_5 = 0x05,   /**< ±62.5 dps, 524.3 LSB/dps. */
    ICM42688_GYRO_FS_31_25 = 0x06,  /**< ±31.25 dps, 1048.6 LSB/dps. */
    ICM42688_GYRO_FS_15_625 = 0x07, /**< ±15.625 dps, 2097.2 LSB/dps. */
} ICM42688_GyroFullScale_t;

/**
 * @brief ICM-42688-P device handle.
 *
 * Holds the transport by value; initialize @c bus (via a port) before
 * calling any driver function.
 */
typedef struct {
    Libdrivers_Bus_t bus;                     /**< Register-bus transport for this device. */
    ICM42688_AccelFullScale_t AccelFullScale; /**< Decoded by ICM42688_Init() from ACCEL_CONFIG0. */
    ICM42688_GyroFullScale_t GyroFullScale;   /**< Decoded by ICM42688_Init() from GYRO_CONFIG0. */
} ICM42688_Handle_t;

/**
 * @brief Control-register values written by ICM42688_Init().
 *
 * Unlike the ODR-powers-on ST parts, the ICM-42688-P keeps each sub-sensor
 * off until PWR_MGMT0 selects a run mode, so power control is a separate
 * field from the ODR/full-scale config registers.
 */
typedef struct {
    uint8_t PwrMgmt0;     /**< PWR_MGMT0 (0x4E): GYRO_MODE + ACCEL_MODE power selects. */
    uint8_t GyroConfig0;  /**< GYRO_CONFIG0 (0x4F): gyro full-scale + ODR. */
    uint8_t AccelConfig0; /**< ACCEL_CONFIG0 (0x50): accel full-scale + ODR. */
} ICM42688_Config_t;

/**
 * @brief Signed 16-bit per-axis accelerometer sample.
 */
typedef struct {
    int16_t X; /**< X axis. */
    int16_t Y; /**< Y axis. */
    int16_t Z; /**< Z axis. */
} ICM42688_AccelData_t;

/**
 * @brief Signed 16-bit per-axis gyroscope sample.
 */
typedef struct {
    int16_t X; /**< X axis. */
    int16_t Y; /**< Y axis. */
    int16_t Z; /**< Z axis. */
} ICM42688_GyroData_t;

/**
 * @brief Scaled per-axis accelerometer sample, in milli-g.
 */
typedef struct {
    int32_t X_mg; /**< X axis, mg. */
    int32_t Y_mg; /**< Y axis, mg. */
    int32_t Z_mg; /**< Z axis, mg. */
} ICM42688_AccelData_mg_t;

/**
 * @brief Scaled per-axis gyroscope sample, in milli-degrees-per-second.
 */
typedef struct {
    int32_t X_mdps; /**< X axis, mdps. */
    int32_t Y_mdps; /**< Y axis, mdps. */
    int32_t Z_mdps; /**< Z axis, mdps. */
} ICM42688_GyroData_mdps_t;

/**
 * @brief Configure the device by writing PWR_MGMT0, GYRO_CONFIG0, and
 *        ACCEL_CONFIG0 from @p pConfig.
 *
 * Stops at the first failing write and returns its status.
 *
 * @param pHandle Handle with an initialized bus.
 * @param pConfig Control-register values to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_Init(ICM42688_Handle_t *pHandle, const ICM42688_Config_t *pConfig);

/**
 * @brief Read @p Length bytes starting at @p RegAddress.
 *
 * Sends the plain register address; the device auto-increments its internal
 * pointer on burst reads, so multi-byte reads walk consecutive registers
 * without setting any address bit.
 *
 * @param pHandle       Handle with an initialized bus.
 * @param RegAddress    First register to read.
 * @param[out] pBuffer  Destination buffer; must hold @p Length bytes.
 * @param Length        Number of bytes to read.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_ReadReg(ICM42688_Handle_t *pHandle, uint8_t RegAddress,
                                     uint8_t *pBuffer, uint16_t Length);

/**
 * @brief Write a single byte to @p RegAddress.
 *
 * @param pHandle    Handle with an initialized bus.
 * @param RegAddress Register to write.
 * @param Value      Byte to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_WriteReg(ICM42688_Handle_t *pHandle, uint8_t RegAddress,
                                      uint8_t Value);

/**
 * @brief Read the raw ADC counts from the X, Y, Z output registers of the accelerometer.
 *
 * @param pHandle     Handle with an initialized bus.
 * @param[out] pData  Filled with the signed per-axis samples.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_ReadRawAccel(ICM42688_Handle_t *pHandle, ICM42688_AccelData_t *pData);

/**
 * @brief Read the raw ADC counts from the X, Y, Z output registers of the gyroscope.
 *
 * @param pHandle     Handle with an initialized bus.
 * @param[out] pData  Filled with the signed per-axis samples.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_ReadRawGyro(ICM42688_Handle_t *pHandle, ICM42688_GyroData_t *pData);

/**
 * @brief Read the accelerometer and scale it to milli-g.
 *
 * Takes a fresh raw sample and scales it using the full-scale range decoded
 * by ICM42688_Init() from ACCEL_CONFIG0 (ICM42688_Handle_t::AccelFullScale).
 *
 * @param pHandle     Handle with an initialized bus, after ICM42688_Init().
 * @param[out] pData  Filled with the signed per-axis samples, in mg.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_ReadAccel_mg(ICM42688_Handle_t *pHandle,
                                          ICM42688_AccelData_mg_t *pData);

/**
 * @brief Read the gyroscope and scale it to milli-degrees-per-second.
 *
 * Takes a fresh raw sample and scales it using the full-scale range decoded
 * by ICM42688_Init() from GYRO_CONFIG0 (ICM42688_Handle_t::GyroFullScale).
 *
 * @param pHandle     Handle with an initialized bus, after ICM42688_Init().
 * @param[out] pData  Filled with the signed per-axis samples, in mdps.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_ReadGyro_mdps(ICM42688_Handle_t *pHandle,
                                           ICM42688_GyroData_mdps_t *pData);

/**
 * @brief Read the on-die temperature sensor and convert it to degrees Celsius.
 *
 * Converts the signed 16-bit TEMP_DATA reading with the datasheet formula
 * degC = (raw / 132.48) + 25.
 *
 * @param pHandle           Handle with an initialized bus.
 * @param[out] pTemperature Filled with the temperature in degrees Celsius.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t ICM42688_ReadTemperature(ICM42688_Handle_t *pHandle, float *pTemperature);

/**
 * @brief Verify the WHO_AM_I register matches ICM42688_WHO_AM_I_VALUE.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK if the ID matches; LIBDRIVERS_ERR_ID on mismatch;
 *         otherwise a transport error.
 */
Libdrivers_Status_t ICM42688_CheckWhoAmI(ICM42688_Handle_t *pHandle);

#endif // ICM42688_H
