#ifndef LIBDRIVERS_BUS_H
#define LIBDRIVERS_BUS_H

#include <stdint.h>

// Bus status enum
typedef enum {
    LIBDRIVERS_OK,         // Success
    LIBDRIVERS_ERR_BUS,    // Transport failed (the read/write hook returned an error)
    LIBDRIVERS_ERR_ARG,    // Bad argument / missing capability (e.g. needed delay but it was NULL)
    LIBDRIVERS_ERR_ID,     // WHO_AM_I mismatch (chip responded, wrong identity)
    LIBDRIVERS_ERR_TIMEOUT // Operation timed out
} Libdrivers_Status_t;

// Function-pointer types
// reg is the plain register address. auto-increment occurs in the sensors driver
// Return LIBDRIVERS_OK on success, LIBDRIVERS_ERR_BUS (or ERR_TIMEOUT) on failure
typedef Libdrivers_Status_t (*Libdrivers_read_fn)(void *ctx, uint8_t reg, uint8_t *buf,
                                                  uint16_t len);
typedef Libdrivers_Status_t (*Libdrivers_write_fn)(void *ctx, uint8_t reg, const uint8_t *buf,
                                                   uint16_t len);
typedef void (*Libdrivers_delay_fn)(void *ctx, uint32_t ms);

// Struct to store function pointers and types
typedef struct {
    Libdrivers_read_fn read;
    Libdrivers_write_fn write;
    Libdrivers_delay_fn delay; // delay can be NULL. if a driver needs delay and it is NULL it
                               // returns LIBDRIVERS_ERR_ARG
    void *ctx; // opaque. core never dereferences it. the app owns ctx and it must outlive the
               // handle (static lifetime), because the driver calls back through it on every read
} Libdrivers_Bus_t;

#endif // LIBDRIVERS_BUS_Hread
