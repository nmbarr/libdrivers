#ifndef LIBDRIVERS_STM32_I2C_H
#define LIBDRIVERS_STM32_I2C_H

#include "libdrivers/bus.h"
#include "stm32l4xx_hal.h" // IWYU pragma: keep
#include <stdint.h>

/**
 * @file libdrivers_stm32_i2c.h
 * @brief STM32 HAL port of the Libdrivers_Bus_t register-bus transport.
 *
 * The only place the STM32 HAL is included. Wraps HAL_I2C_Mem_Read/Write and
 * HAL_Delay so any register-bus driver runs on STM32 I2C hardware. Built by
 * the firmware project (which owns the HAL), not by this library's CMake.
 */

/**
 * @brief Port context: which I2C peripheral and device address to use.
 *
 * Referenced by Libdrivers_Bus_t::ctx. The app owns it; it must outlive the
 * handle (static lifetime), since the driver calls back through it on every
 * access.
 */
typedef struct {
    I2C_HandleTypeDef *hi2c; /**< STM32 HAL I2C peripheral handle. */
    uint8_t device_addr;     /**< 7-bit device address, HAL-shifted as required. */
} Libdrivers_STM32_I2C_Context_t;

/**
 * @brief Wire an STM32 I2C context into a bus by installing the hooks.
 *
 * Sets @p bus->read / write / delay to the STM32 adapters and @p bus->ctx to
 * @p context. Call once before handing the bus to a driver.
 *
 * @param[out] bus  Bus struct to populate.
 * @param context   Port context; must outlive @p bus.
 */
void Libdrivers_STM32_I2C_InitBus(Libdrivers_Bus_t *bus, Libdrivers_STM32_I2C_Context_t *context);

#endif // LIBDRIVERS_STM32_I2C_H
