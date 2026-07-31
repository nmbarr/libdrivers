#include "libdrivers/bus.h"
#include <stdint.h>

Libdrivers_Status_t Libdrivers_Bus_CheckWhoAmI(Libdrivers_Bus_t *bus, uint8_t reg,
                                               uint8_t expected) {

    // Byte to hold the identity register's contents
    uint8_t WhoAmIByte;

    // Read one byte straight from the bus; a 1-byte read needs no auto-increment
    Libdrivers_Status_t status = bus->read(bus->ctx, reg, &WhoAmIByte, 1);
    if (status != LIBDRIVERS_OK) {
        return status; // Propagate the transport error
    }

    // Read succeeded. Check the ID
    if (WhoAmIByte != expected) {
        return LIBDRIVERS_ERR_ID;
    }

    // Correct chip. Return OK
    return LIBDRIVERS_OK;
}
