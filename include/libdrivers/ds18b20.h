#ifndef DS18B20_H
#define DS18B20_H

#include "libdrivers/onewire.h"

/**
 * @file ds18b20.h
 * @brief Driver for the Maxim DS18B20 1-Wire digital thermometer.
 *
 * Unlike the register-bus sensors, the DS18B20 speaks Maxim's 1-Wire protocol,
 * so it is built against Libdrivers_OneWire_t (onewire.h) rather than
 * Libdrivers_Bus_t. This is the single-device variant: every transaction uses
 * Skip ROM, so exactly one DS18B20 may be present on the line.
 *
 * A temperature reading is a two-step protocol, because a 12-bit conversion
 * takes up to 750 ms: call DS18B20_StartConversion() to kick off a measurement,
 * wait for the conversion to finish (the caller owns that delay -- the driver
 * has no delay hook), then call DS18B20_ReadTemperature() to fetch the result.
 */

// ROM + function commands
#define DS18B20_CMD_SKIP_ROM        0xCC
#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

/**
 * @brief DS18B20 device handle.
 *
 * Holds the 1-Wire transport by value. Initialize @c ow (via a port) before
 * use; every driver call reaches the device through it.
 */
typedef struct {
    Libdrivers_OneWire_t ow; /**< 1-Wire transport for this device. */
} DS18B20_Handle_t;

/**
 * @brief Confirm a DS18B20 is present on the line.
 *
 * Issues a 1-Wire reset and checks for a presence pulse. Since 1-Wire devices
 * have no WHO_AM_I register, this handshake is the stand-in for an ID check.
 *
 * @param pHandle Handle with an initialized transport.
 * @return LIBDRIVERS_OK if a device responded; LIBDRIVERS_ERR_BUS if none did.
 */
Libdrivers_Status_t DS18B20_Init(DS18B20_Handle_t *pHandle);

/**
 * @brief Start a temperature conversion, then return immediately.
 *
 * Sends reset + Skip ROM + Convert T. The conversion runs on the device and
 * takes up to 750 ms for 12-bit resolution; this call does @e not wait for it.
 * The caller must delay for the conversion time before DS18B20_ReadTemperature()
 * will return a fresh sample.
 *
 * @param pHandle Handle with an initialized transport.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t DS18B20_StartConversion(DS18B20_Handle_t *pHandle);

/**
 * @brief Read the most recent conversion result in degrees Celsius.
 *
 * Sends reset + Skip ROM + Read Scratchpad, reads the two temperature bytes,
 * and decodes them to degrees C. Assumes a prior DS18B20_StartConversion() has
 * had time to complete; otherwise it returns whatever the scratchpad holds.
 *
 * @param pHandle            Handle with an initialized transport.
 * @param[out] pTemperature  Written with the temperature in degrees C on
 *                           success; left unchanged on failure.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
Libdrivers_Status_t DS18B20_ReadTemperature(DS18B20_Handle_t *pHandle, float *pTemperature);

#endif // DS18B20_H
