#include "libdrivers/ds18b20.h"
#include <stdint.h>

Libdrivers_Status_t DS18B20_Init(DS18B20_Handle_t *pHandle) {
    return pHandle->ow.reset(pHandle->ow.ctx);
}

Libdrivers_Status_t DS18B20_StartConversion(DS18B20_Handle_t *pHandle) {

    // Status from each transport step
    Libdrivers_Status_t status;

    // Verify that the presence pulse was received. If not, early return
    status = pHandle->ow.reset(pHandle->ow.ctx);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Send the SKIP_ROM cmd (Only one device will be on the wire so no need to read the 64-bit ROM
    // code)
    status = pHandle->ow.write(pHandle->ow.ctx, DS18B20_CMD_SKIP_ROM);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Send Convert T — start a temperature measurement (runs on the device, up to 750ms)
    status = pHandle->ow.write(pHandle->ow.ctx, DS18B20_CMD_CONVERT_T);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t DS18B20_ReadTemperature(DS18B20_Handle_t *pHandle, float *pTemperature) {

    // Status from each transport step
    Libdrivers_Status_t status;

    // Verify that the presence pulse was received. If not, early return
    status = pHandle->ow.reset(pHandle->ow.ctx);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Send the SKIP_ROM cmd (Only one device will be on the wire so no need to read the 64-bit ROM
    // code)
    status = pHandle->ow.write(pHandle->ow.ctx, DS18B20_CMD_SKIP_ROM);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Send the READ_SCRATCHPAD cmd (Read the contents of the scratchpad)
    status = pHandle->ow.write(pHandle->ow.ctx, DS18B20_CMD_READ_SCRATCHPAD);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    uint8_t lsb, msb;

    status = pHandle->ow.read(pHandle->ow.ctx, &lsb);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = pHandle->ow.read(pHandle->ow.ctx, &msb);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Read the raw two's complement temperature
    int16_t raw = (int16_t)((msb << 8) | lsb);

    // Convert to Celsius
    *pTemperature = raw / 16.0f;

    return LIBDRIVERS_OK;
}
