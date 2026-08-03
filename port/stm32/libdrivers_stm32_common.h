#ifndef LIBDRIVERS_STM32_COMMON_H
#define LIBDRIVERS_STM32_COMMON_H

#include "libdrivers/bus.h"
#include "stm32l4xx_hal.h" // IWYU pragma: keep

/**
 * @file libdrivers_stm32_common.h
 * @brief Shared helpers for the STM32 HAL ports.
 *
 * Small utilities common to more than one STM32 register-bus port (I2C, SPI).
 * Lives in port/ because it names the vendor HAL type HAL_StatusTypeDef, which
 * must never reach the HAL-free core.
 */

/**
 * @brief Translate an STM32 HAL status into a Libdrivers_Status_t.
 *
 * Maps HAL_OK to LIBDRIVERS_OK and HAL_TIMEOUT to LIBDRIVERS_ERR_TIMEOUT;
 * every other HAL status collapses to LIBDRIVERS_ERR_BUS.
 *
 * @param HalStatus Status returned by a HAL transfer call.
 * @return The equivalent Libdrivers_Status_t.
 */
Libdrivers_Status_t Libdrivers_STM32_CollapseStatus(HAL_StatusTypeDef HalStatus);

#endif // LIBDRIVERS_STM32_COMMON_H
