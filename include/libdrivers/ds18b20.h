#ifndef DS18B20_H
#define DS18B20_H

#include "libdrivers/bus.h"
#include <stdint.h>

/**
 * @file ds18b20.h
 * @brief Driver for the Maxim DS18B20 1-Wire digital thermometer.
 *
 * Stub. Unlike the register-bus sensors, the DS18B20 speaks Maxim's 1-Wire
 * protocol, so it will be built against Libdrivers_OneWire_t (onewire.h)
 * rather than Libdrivers_Bus_t. Planned single-device / Skip ROM API:
 * presence-check init, start-conversion, and read-temperature.
 */

#endif // DS18B20_H
