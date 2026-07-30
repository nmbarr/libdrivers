#ifndef LIBDRIVERS_ONEWIRE_H
#define LIBDRIVERS_ONEWIRE_H

#include "libdrivers/bus.h"
#include <stdint.h>

/**
 * @file onewire.h
 * @brief Vendor-agnostic 1-Wire transport contract.
 *
 * Sibling to bus.h, but for Maxim's 1-Wire protocol instead of register
 * buses (I2C/SPI). A driver for a 1-Wire device (e.g. DS18B20) depends on
 * this interface; a port implements the three hooks against real hardware
 * and hides all sub-microsecond bit timing. The core never sees a bit slot
 * or a timing constant -- only reset / write / read.
 *
 * Status values are reused from bus.h (Libdrivers_Status_t) so a device
 * driver returns one status type regardless of which transport it speaks.
 */

/**
 * @brief Issue a 1-Wire reset pulse and sample for a presence pulse.
 *
 * Drives the line low for the reset duration, releases it, then listens for
 * the device's presence pulse. Because 1-Wire devices have no WHO_AM_I
 * register, this handshake is also how a driver confirms a device is on the
 * line at all.
 *
 * @param ctx Opaque port context (see Libdrivers_OneWire_t::ctx).
 * @return LIBDRIVERS_OK if a presence pulse was detected;
 *         LIBDRIVERS_ERR_BUS if no device responded.
 */
typedef Libdrivers_Status_t (*Libdrivers_ow_reset_fn)(void *ctx);

/**
 * @brief Write one byte to the line, least-significant bit first.
 *
 * The port emits the eight write time-slots; bit ordering and slot timing
 * are the port's responsibility, not the caller's.
 *
 * @param ctx  Opaque port context.
 * @param byte The byte to transmit.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
typedef Libdrivers_Status_t (*Libdrivers_ow_write_byte_fn)(void *ctx, uint8_t byte);

/**
 * @brief Read one byte from the line, least-significant bit first.
 *
 * The port drives the eight read time-slots and samples each bit.
 *
 * @param ctx       Opaque port context.
 * @param[out] byte Destination for the received byte; written only on
 *                  LIBDRIVERS_OK.
 * @return LIBDRIVERS_OK on success, or a transport error.
 */
typedef Libdrivers_Status_t (*Libdrivers_ow_read_byte_fn)(void *ctx, uint8_t *byte);

/**
 * @brief A 1-Wire transport: three hooks plus an opaque port context.
 *
 * A port fills this in (e.g. a bit-banged GPIO implementation), and a device
 * driver holds one by value and calls through the hooks. There is no delay
 * hook: bit timing lives in the port, and a device driver handles longer
 * waits (such as temperature-conversion time) itself.
 */
typedef struct {
    Libdrivers_ow_reset_fn reset;      /**< Reset + presence detect. */
    Libdrivers_ow_write_byte_fn write; /**< Write one byte, LSB first. */
    Libdrivers_ow_read_byte_fn read;   /**< Read one byte, LSB first. */
    void *ctx; /**< Opaque, port-owned. The core never dereferences it; the
                    app owns it and it must outlive the handle. */
} Libdrivers_OneWire_t;

#endif // LIBDRIVERS_ONEWIRE_H
