#ifndef LIBDRIVERS_STM32_ONEWIRE_H
#define LIBDRIVERS_STM32_ONEWIRE_H

#include "libdrivers/onewire.h"
#include "stm32l4xx_hal.h" // IWYU pragma: keep
#include <stdint.h>

/**
 * @file libdrivers_stm32_onewire.h
 * @brief STM32 bit-banged GPIO port of the Libdrivers_OneWire_t transport.
 *
 * One of the two places the STM32 HAL is included. Drives a single GPIO pin to
 * speak Maxim's 1-Wire protocol, so any 1-Wire driver (e.g. DS18B20) runs on
 * STM32 hardware. This port owns all sub-microsecond bit timing, generated in
 * software from the DWT cycle counter; the driver above it never sees a bit
 * slot or a timing constant. Built by the firmware project (which owns the
 * HAL), not by this library's CMake.
 */

/**
 * @brief Port context: which GPIO port and pin carry the 1-Wire line.
 *
 * Referenced by Libdrivers_OneWire_t::ctx. The app owns it; it must outlive the
 * handle (static lifetime), since the port calls back through it on every
 * access. The caller is responsible for configuring the pin as open-drain with
 * a pull-up and enabling the DWT cycle counter before wiring this into a
 * transport -- this port only drives the already-configured pin.
 */
typedef struct {
    GPIO_TypeDef *port; /**< GPIO port for the 1-Wire line. */
    uint16_t pin;       /**< GPIO pin mask (e.g. GPIO_PIN_4). */
} Libdrivers_STM32_OneWire_Context_t;

/**
 * @brief Wire an STM32 GPIO context into a 1-Wire transport by installing the
 *        hooks.
 *
 * Sets @p ow->reset / write / read to the bit-banged adapters and @p ow->ctx to
 * @p context. Call once before handing the transport to a driver. The pin in
 * @p context must already be configured (open-drain, pull-up) and DWT enabled.
 *
 * @param[out] ow  Transport struct to populate.
 * @param context  Port context; must outlive @p ow.
 */
void Libdrivers_STM32_OneWire_InitBus(Libdrivers_OneWire_t *ow,
                                      Libdrivers_STM32_OneWire_Context_t *context);

#endif // LIBDRIVERS_STM32_ONEWIRE_H
