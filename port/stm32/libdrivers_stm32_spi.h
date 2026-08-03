#ifndef LIBDRIVERS_STM32_SPI_H
#define LIBDRIVERS_STM32_SPI_H

#include "libdrivers/bus.h"
#include "stm32l4xx_hal.h" // IWYU pragma: keep
#include <stdint.h>

/**
 * @file libdrivers_stm32_spi.h
 * @brief STM32 HAL port of the Libdrivers_Bus_t register-bus transport (SPI).
 *
 * The only place the STM32 HAL is included. Wraps HAL_SPI_Transmit/Receive
 * with a manual chip-select GPIO, plus HAL_Delay, so any register-bus driver
 * runs on STM32 SPI hardware. Applies the SPI R/W bit (bit 7 of the address:
 * 1 = read, 0 = write); the driver passes the plain register address. Built by
 * the firmware project (which owns the HAL), not by this library's CMake.
 */

/**
 * @brief Port context: which SPI peripheral and chip-select line to use.
 *
 * Referenced by Libdrivers_Bus_t::ctx. The app owns it; it must outlive the
 * handle (static lifetime), since the driver calls back through it on every
 * access.
 */
typedef struct {
    SPI_HandleTypeDef *hspi; /**< STM32 HAL SPI peripheral handle. */
    GPIO_TypeDef *cs_port;   /**< Chip-select GPIO port. */
    uint16_t cs_pin;         /**< Chip-select pin mask (GPIO_PIN_x). */
} Libdrivers_STM32_SPI_Context_t;

/**
 * @brief Wire an STM32 SPI context into a bus by installing the hooks.
 *
 * Sets @p bus->read / write / delay to the STM32 SPI adapters and @p bus->ctx
 * to @p context. Call once before handing the bus to a driver.
 *
 * @param[out] bus  Bus struct to populate.
 * @param context   Port context; must outlive @p bus.
 */
void Libdrivers_STM32_SPI_InitBus(Libdrivers_Bus_t *bus, Libdrivers_STM32_SPI_Context_t *context);

#endif // LIBDRIVERS_STM32_SPI_H
