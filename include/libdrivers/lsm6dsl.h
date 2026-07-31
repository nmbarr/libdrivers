#ifndef LSM6DSL_H
#define LSM6DSL_H

#include "libdrivers/bus.h"
#include <stdint.h>

/**
 * @file lsm6dsl.h
 * @brief Driver for the ST LSM6DSL accelerometer + gyroscope (register bus).
 *
 * Speaks to the device through a Libdrivers_Bus_t transport, so the driver is
 * HAL-free: a port supplies the actual I2C/SPI access. The accelerometer and
 * gyroscope are independent sub-sensors with their own control and output
 * registers, so each is configured and read separately. Multi-byte reads rely
 * on the device's own address auto-increment (IF_INC in CTRL3_C, on by
 * default), so no address bit is set per transfer.
 */

// Register addresses
#define LSM6DSL_REG_FUNC_CFG_ACCESS        0x01
#define LSM6DSL_REG_SENSOR_SYNC_TIME_FRAME 0x04
#define LSM6DSL_REG_SENSOR_SYNC_RES_RATIO  0x05
#define LSM6DSL_REG_FIFO_CTRL1             0x06
#define LSM6DSL_REG_FIFO_CTRL2             0x07
#define LSM6DSL_REG_FIFO_CTRL3             0x08
#define LSM6DSL_REG_FIFO_CTRL4             0x09
#define LSM6DSL_REG_FIFO_CTRL5             0x0A
#define LSM6DSL_REG_DRDY_PULSE_CFG_G       0x0B
#define LSM6DSL_REG_INT1_CTRL              0x0D
#define LSM6DSL_REG_INT2_CTRL              0x0E
#define LSM6DSL_REG_WHO_AM_I               0x0F
#define LSM6DSL_REG_CTRL1_XL               0x10
#define LSM6DSL_REG_CTRL2_G                0x11
#define LSM6DSL_REG_CTRL3_C                0x12
#define LSM6DSL_REG_CTRL4_C                0x13
#define LSM6DSL_REG_CTRL5_C                0x14
#define LSM6DSL_REG_CTRL6_C                0x15
#define LSM6DSL_REG_CTRL7_G                0x16
#define LSM6DSL_REG_CTRL8_XL               0x17
#define LSM6DSL_REG_CTRL9_XL               0x18
#define LSM6DSL_REG_CTRL10_C               0x19

#define LSM6DSL_REG_MASTER_CONFIG          0x1A
#define LSM6DSL_REG_WAKE_UP_SRC            0x1B
#define LSM6DSL_REG_TAP_SRC                0x1C
#define LSM6DSL_REG_D6D_SRC                0x1D
#define LSM6DSL_REG_STATUS_REG             0x1E

#define LSM6DSL_REG_OUT_TEMP_L             0x20
#define LSM6DSL_REG_OUT_TEMP_H             0x21
#define LSM6DSL_REG_OUTX_L_G               0x22
#define LSM6DSL_REG_OUTX_H_G               0x23
#define LSM6DSL_REG_OUTY_L_G               0x24
#define LSM6DSL_REG_OUTY_H_G               0x25
#define LSM6DSL_REG_OUTZ_L_G               0x26
#define LSM6DSL_REG_OUTZ_H_G               0x27
#define LSM6DSL_REG_OUTX_L_XL              0x28
#define LSM6DSL_REG_OUTX_H_XL              0x29
#define LSM6DSL_REG_OUTY_L_XL              0x2A
#define LSM6DSL_REG_OUTY_H_XL              0x2B
#define LSM6DSL_REG_OUTZ_L_XL              0x2C
#define LSM6DSL_REG_OUTZ_H_XL              0x2D

#define LSM6DSL_REG_SENSORHUB1_REG         0x2E
#define LSM6DSL_REG_SENSORHUB2_REG         0x2F
#define LSM6DSL_REG_SENSORHUB3_REG         0x30
#define LSM6DSL_REG_SENSORHUB4_REG         0x31
#define LSM6DSL_REG_SENSORHUB5_REG         0x32
#define LSM6DSL_REG_SENSORHUB6_REG         0x33
#define LSM6DSL_REG_SENSORHUB7_REG         0x34
#define LSM6DSL_REG_SENSORHUB8_REG         0x35
#define LSM6DSL_REG_SENSORHUB9_REG         0x36
#define LSM6DSL_REG_SENSORHUB10_REG        0x37
#define LSM6DSL_REG_SENSORHUB11_REG        0x38
#define LSM6DSL_REG_SENSORHUB12_REG        0x39

#define LSM6DSL_REG_FIFO_STATUS1           0x3A
#define LSM6DSL_REG_FIFO_STATUS2           0x3B
#define LSM6DSL_REG_FIFO_STATUS3           0x3C
#define LSM6DSL_REG_FIFO_STATUS4           0x3D
#define LSM6DSL_REG_FIFO_DATA_OUT_L        0x3E
#define LSM6DSL_REG_FIFO_DATA_OUT_H        0x3F

#define LSM6DSL_REG_TIMESTAMP0_REG         0x40
#define LSM6DSL_REG_TIMESTAMP1_REG         0x41
#define LSM6DSL_REG_TIMESTAMP2_REG         0x42

#define LSM6DSL_REG_STEP_TIMESTAMP_L       0x49
#define LSM6DSL_REG_STEP_TIMESTAMP_H       0x4A
#define LSM6DSL_REG_STEP_COUNTER_L         0x4B
#define LSM6DSL_REG_STEP_COUNTER_H         0x4C

#define LSM6DSL_REG_SENSORHUB13_REG        0x4D
#define LSM6DSL_REG_SENSORHUB14_REG        0x4E
#define LSM6DSL_REG_SENSORHUB15_REG        0x4F
#define LSM6DSL_REG_SENSORHUB16_REG        0x50
#define LSM6DSL_REG_SENSORHUB17_REG        0x51
#define LSM6DSL_REG_SENSORHUB18_REG        0x52

#define LSM6DSL_REG_FUNC_SRC1              0x53
#define LSM6DSL_REG_FUNC_SRC2              0x54
#define LSM6DSL_REG_WRIST_TILT_IA          0x55

#define LSM6DSL_REG_TAP_CFG                0x58
#define LSM6DSL_REG_TAP_THS_6D             0x59
#define LSM6DSL_REG_INT_DUR2               0x5A
#define LSM6DSL_REG_WAKE_UP_THS            0x5B
#define LSM6DSL_REG_WAKE_UP_DUR            0x5C
#define LSM6DSL_REG_FREE_FALL              0x5D
#define LSM6DSL_REG_MD1_CFG                0x5E
#define LSM6DSL_REG_MD2_CFG                0x5F

#define LSM6DSL_REG_MASTER_CMD_CODE        0x60
#define LSM6DSL_REG_SENS_SYNC_SPI_ERROR    0x61

#define LSM6DSL_REG_OUT_MAG_RAW_X_L        0x62
#define LSM6DSL_REG_OUT_MAG_RAW_X_H        0x63
#define LSM6DSL_REG_OUT_MAG_RAW_Y_L        0x64
#define LSM6DSL_REG_OUT_MAG_RAW_Y_H        0x65
#define LSM6DSL_REG_OUT_MAG_RAW_Z_L        0x66
#define LSM6DSL_REG_OUT_MAG_RAW_Z_H        0x67

#define LSM6DSL_REG_X_OFS_USR              0x73
#define LSM6DSL_REG_Y_OFS_USR              0x74
#define LSM6DSL_REG_Z_OFS_USR              0x75

// Expected WHO_AM_I value
#define LSM6DSL_WHO_AM_I_VALUE 0x6A

/**
 * @brief LSM6DSL device handle.
 *
 * Holds the transport by value; initialize @c bus (via a port) before
 * calling any driver function.
 */
typedef struct {
    Libdrivers_Bus_t bus; /**< Register-bus transport for this device. */
} LSM6DSL_Handle_t;

/**
 * @brief Control-register values written by LSM6DSL_Init().
 */
typedef struct {
    uint8_t CtrlReg1_XL; /**< CTRL1_XL (0x10): accelerometer ODR + full-scale. */
    uint8_t CtrlReg2_G;  /**< CTRL2_G (0x11): gyroscope ODR + full-scale. */
    uint8_t CtrlReg3_C;  /**< CTRL3_C (0x12): common config; keep IF_INC set. */
} LSM6DSL_Config_t;

/**
 * @brief Signed 16-bit per-axis accelerometer sample.
 */
typedef struct {
    int16_t X; /**< X axis. */
    int16_t Y; /**< Y axis. */
    int16_t Z; /**< Z axis. */
} LSM6DSL_XLData_t;

/**
 * @brief Signed 16-bit per-axis gyroscope sample.
 */
typedef struct {
    int16_t X; /**< X axis. */
    int16_t Y; /**< Y axis. */
    int16_t Z; /**< Z axis. */
} LSM6DSL_GyroData_t;

/**
 * @brief Configure the device by writing CTRL_REG1..3 from @p pConfig.
 *
 * Stops at the first failing write and returns its status.
 *
 * @param pHandle Handle with an initialized bus.
 * @param pConfig Control-register values to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LSM6DSL_Init(LSM6DSL_Handle_t *pHandle, const LSM6DSL_Config_t *pConfig);

/**
 * @brief Read @p Length bytes starting at @p RegAddress.
 *
 * Sends the plain register address; the device auto-increments its internal
 * pointer (IF_INC in CTRL3_C, enabled at init), so multi-byte reads walk
 * consecutive registers without setting any address bit.
 *
 * @param pHandle       Handle with an initialized bus.
 * @param RegAddress    First register to read.
 * @param[out] pBuffer  Destination buffer; must hold @p Length bytes.
 * @param Length        Number of bytes to read.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LSM6DSL_ReadReg(LSM6DSL_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                    uint16_t Length);

/**
 * @brief Write a single byte to @p RegAddress.
 *
 * @param pHandle    Handle with an initialized bus.
 * @param RegAddress Register to write.
 * @param Value      Byte to write.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LSM6DSL_WriteReg(LSM6DSL_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value);

/**
 * @brief Read the raw ADC counts from the X, Y, Z output registers of the accelerometer.
 *
 * @param pHandle     Handle with an initialized bus.
 * @param[out] pData  Filled with the signed per-axis samples.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LSM6DSL_ReadRawXL(LSM6DSL_Handle_t *pHandle, LSM6DSL_XLData_t *pData);

/**
 * @brief Read the raw ADC counts from the X, Y, Z output registers of the gyroscope.
 *
 * @param pHandle     Handle with an initialized bus.
 * @param[out] pData  Filled with the signed per-axis samples.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LSM6DSL_ReadRawGyro(LSM6DSL_Handle_t *pHandle, LSM6DSL_GyroData_t *pData);

/**
 * @brief Verify the WHO_AM_I register matches LSM6DSL_WHO_AM_I_VALUE.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK if the ID matches; LIBDRIVERS_ERR_ID on mismatch;
 *         otherwise a transport error.
 */
Libdrivers_Status_t LSM6DSL_CheckWhoAmI(LSM6DSL_Handle_t *pHandle);

#endif // LSM6DSL_H
