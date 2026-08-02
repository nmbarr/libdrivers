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

// Bit mask values
#define LSM6DSL_FS_XL_BIT_MASK 0x0C /**< FS_XL[1:0] field within CTRL1_XL. */
#define LSM6DSL_FS_G_BIT_MASK  0x0C /**< FS_G[1:0] field within CTRL2_G. */
#define LSM6DSL_FS_125_BIT_MASK                                                                    \
    0x02 /**< FS_125 bit within CTRL2_G; overrides FS_G[1:0] when set. */

// Bit shift values
#define LSM6DSL_FS_XL_BIT_SHIFT 2 /**< Shift to right-align FS_XL[1:0] after masking. */
#define LSM6DSL_FS_G_BIT_SHIFT  2 /**< Shift to right-align FS_G[1:0] after masking. */

// Expected WHO_AM_I value
#define LSM6DSL_WHO_AM_I_VALUE 0x6A

/**
 * @brief Accelerometer full-scale range (FS_XL[1:0] in CTRL1_XL).
 *
 * Enumerator values equal the raw 2-bit FS_XL code, so decoding is a direct
 * cast of the masked, shifted register value. Note the non-ascending order:
 * the hardware encoding is not simply 2/4/8/16.
 */
typedef enum {
    LSM6DSL_FS_XL_2G = 0x00,  /**< FS_XL = 00b: +/-2 g, 0.061 mg/LSB. */
    LSM6DSL_FS_XL_16G = 0x01, /**< FS_XL = 01b: +/-16 g, 0.488 mg/LSB. */
    LSM6DSL_FS_XL_4G = 0x02,  /**< FS_XL = 10b: +/-4 g, 0.122 mg/LSB. */
    LSM6DSL_FS_XL_8G = 0x03,  /**< FS_XL = 11b: +/-8 g, 0.244 mg/LSB. */
} LSM6DSL_XLFullScale_t;

/**
 * @brief Gyroscope full-scale range (FS_G[1:0] and FS_125 in CTRL2_G).
 *
 * The first four enumerator values equal the raw 2-bit FS_G code, so
 * decoding is a direct cast of the masked, shifted register value. 125 dps
 * is selected by the separate FS_125 bit (overriding FS_G[1:0]), so it is
 * given a value outside that 2-bit range.
 */
typedef enum {
    LSM6DSL_FS_G_250 = 0x00,  /**< FS_G = 00b: +/-250 dps, 8.75 mdps/LSB. */
    LSM6DSL_FS_G_500 = 0x01,  /**< FS_G = 01b: +/-500 dps, 17.50 mdps/LSB. */
    LSM6DSL_FS_G_1000 = 0x02, /**< FS_G = 10b: +/-1000 dps, 35 mdps/LSB. */
    LSM6DSL_FS_G_2000 = 0x03, /**< FS_G = 11b: +/-2000 dps, 70 mdps/LSB. */
    LSM6DSL_FS_G_125 = 0x04,  /**< FS_125 = 1: +/-125 dps, 4.375 mdps/LSB. */
} LSM6DSL_GyroFullScale_t;

/**
 * @brief LSM6DSL device handle.
 *
 * Holds the transport by value; initialize @c bus (via a port) before
 * calling any driver function.
 */
typedef struct {
    Libdrivers_Bus_t bus;                  /**< Register-bus transport for this device. */
    LSM6DSL_XLFullScale_t XLFullScale;     /**< Decoded by LSM6DSL_Init() from CTRL1_XL. */
    LSM6DSL_GyroFullScale_t GyroFullScale; /**< Decoded by LSM6DSL_Init() from CTRL2_G. */
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
 * @brief Scaled per-axis accelerometer sample, in milli-g.
 */
typedef struct {
    int32_t X_mg; /**< X axis, mg. */
    int32_t Y_mg; /**< Y axis, mg. */
    int32_t Z_mg; /**< Z axis, mg. */
} LSM6DSL_XLData_mg_t;

/**
 * @brief Scaled per-axis gyroscope sample, in milli-degrees-per-second.
 */
typedef struct {
    int32_t X_mdps; /**< X axis, mdps. */
    int32_t Y_mdps; /**< Y axis, mdps. */
    int32_t Z_mdps; /**< Z axis, mdps. */
} LSM6DSL_GyroData_mdps_t;

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
 * @brief Read the accelerometer and scale it to milli-g.
 *
 * Takes a fresh raw sample and scales it using the full-scale range decoded
 * by LSM6DSL_Init() from CTRL1_XL (LSM6DSL_Handle_t::XLFullScale).
 *
 * @param pHandle     Handle with an initialized bus, after LSM6DSL_Init().
 * @param[out] pData  Filled with the signed per-axis samples, in mg.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LSM6DSL_ReadXL_mg(LSM6DSL_Handle_t *pHandle, LSM6DSL_XLData_mg_t *pData);

/**
 * @brief Read the gyroscope and scale it to milli-degrees-per-second.
 *
 * Takes a fresh raw sample and scales it using the full-scale range decoded
 * by LSM6DSL_Init() from CTRL2_G (LSM6DSL_Handle_t::GyroFullScale).
 *
 * @param pHandle     Handle with an initialized bus, after LSM6DSL_Init().
 * @param[out] pData  Filled with the signed per-axis samples, in mdps.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t LSM6DSL_ReadGyro_mdps(LSM6DSL_Handle_t *pHandle,
                                          LSM6DSL_GyroData_mdps_t *pData);

/**
 * @brief Verify the WHO_AM_I register matches LSM6DSL_WHO_AM_I_VALUE.
 *
 * @param pHandle Handle with an initialized bus.
 * @return LIBDRIVERS_OK if the ID matches; LIBDRIVERS_ERR_ID on mismatch;
 *         otherwise a transport error.
 */
Libdrivers_Status_t LSM6DSL_CheckWhoAmI(LSM6DSL_Handle_t *pHandle);

#endif // LSM6DSL_H
