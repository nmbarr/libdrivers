#ifndef LIBDRIVERS_BUS_H
#define LIBDRIVERS_BUS_H

#include <stdint.h>

/**
 * @file bus.h
 * @brief Vendor-agnostic register-bus transport contract.
 *
 * The keystone of the HAL-free core. A register-based device driver
 * (I2C/SPI: HTS221, LIS3MDL, LSM6DSL) depends on this interface; a port
 * implements the hooks against a real peripheral (e.g. the STM32 HAL) and
 * the core never sees a vendor header. Also defines Libdrivers_Status_t,
 * the single status type shared by every transport (see onewire.h).
 */

/**
 * @brief Result of a bus operation, shared across all drivers and transports.
 */
typedef enum {
    LIBDRIVERS_OK,         /**< Success. */
    LIBDRIVERS_ERR_BUS,    /**< Transport failed (a read/write hook errored). */
    LIBDRIVERS_ERR_ARG,    /**< Bad argument / missing capability (e.g. a delay
                                was needed but the hook was NULL). */
    LIBDRIVERS_ERR_ID,     /**< WHO_AM_I mismatch (chip responded, wrong identity). */
    LIBDRIVERS_ERR_TIMEOUT /**< Operation timed out. */
} Libdrivers_Status_t;

/**
 * @brief Read @p len bytes starting at register @p reg.
 *
 * @p reg is the plain register address; any auto-increment bit is applied
 * by the device driver, not the port.
 *
 * @param ctx       Opaque port context (see Libdrivers_Bus_t::ctx).
 * @param reg       Register address to read from.
 * @param[out] buf  Destination buffer; must hold at least @p len bytes.
 * @param len       Number of bytes to read.
 * @return LIBDRIVERS_OK on success, LIBDRIVERS_ERR_BUS or
 *         LIBDRIVERS_ERR_TIMEOUT on failure.
 */
typedef Libdrivers_Status_t (*Libdrivers_read_fn)(void *ctx, uint8_t reg, uint8_t *buf,
                                                  uint16_t len);

/**
 * @brief Write @p len bytes starting at register @p reg.
 *
 * @param ctx      Opaque port context.
 * @param reg      Register address to write to.
 * @param buf      Source buffer of @p len bytes.
 * @param len      Number of bytes to write.
 * @return LIBDRIVERS_OK on success, LIBDRIVERS_ERR_BUS or
 *         LIBDRIVERS_ERR_TIMEOUT on failure.
 */
typedef Libdrivers_Status_t (*Libdrivers_write_fn)(void *ctx, uint8_t reg, const uint8_t *buf,
                                                   uint16_t len);

/**
 * @brief Block for @p ms milliseconds.
 *
 * @param ctx Opaque port context.
 * @param ms  Delay duration in milliseconds.
 */
typedef void (*Libdrivers_delay_fn)(void *ctx, uint32_t ms);

/**
 * @brief A register bus: read/write/delay hooks plus an opaque port context.
 *
 * A port fills this in and a device driver holds one by value, calling
 * through the hooks.
 */
typedef struct {
    Libdrivers_read_fn read;   /**< Register read hook. */
    Libdrivers_write_fn write; /**< Register write hook. */
    Libdrivers_delay_fn delay; /**< Optional. May be NULL; a driver that needs a
                                    delay while this is NULL returns
                                    LIBDRIVERS_ERR_ARG rather than skipping it. */
    void *ctx;                 /**< Opaque, port-owned. The core never dereferences it; the app
                                    owns it and it must outlive the handle (static lifetime),
                                    since the driver calls back through it on every access. */
} Libdrivers_Bus_t;

/**
 * @brief Read a one-byte identity register and compare it to an expected value.
 *
 * Shared WHO_AM_I helper for register-bus sensors. Reads a single byte from
 * @p reg through @p bus and checks it against @p expected. Uses the bus read
 * hook directly, so no auto-increment bit applies (the read is one byte).
 * Each sensor's public @c XXX_CheckWhoAmI wraps this with its own register and
 * ID constant.
 *
 * @param bus       Initialized register bus.
 * @param reg       Identity register address (e.g. WHO_AM_I).
 * @param expected  Expected register value for this chip.
 * @return LIBDRIVERS_OK if the byte matches; LIBDRIVERS_ERR_ID on mismatch;
 *         otherwise the transport error from the read.
 */
Libdrivers_Status_t Libdrivers_Bus_CheckWhoAmI(Libdrivers_Bus_t *bus, uint8_t reg,
                                               uint8_t expected);

#endif // LIBDRIVERS_BUS_H
