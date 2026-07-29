#ifndef LIBDRIVERS_STM32_I2C_H
#define LIBDRIVERS_STM32_I2C_H

#include "libdrivers/bus.h"
#include "stm32l4xx_hal.h" // IWYU pragma: keep
#include <stdint.h>

// Struct to hold context and device address
// App owns this; must have static lifetime — the driver holds bus.ctx pointing at it.
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t device_addr;
} Libdrivers_STM32_I2C_Context_t;

// Initialize the I2C bus
void Libdrivers_STM32_I2C_InitBus(Libdrivers_Bus_t *bus, Libdrivers_STM32_I2C_Context_t *context);

#endif // LIBDRIVERS_STM32_I2C_H
