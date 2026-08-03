#include "libdrivers/lps22hb.h"
#include <stdint.h>

// Pressure sensitivity: raw counts per hPa
static const float LPS22HB_PRESSURE_COUNTS_PER_HPA = 4096.0f;

// Temperature sensitivity: raw counts per degC
static const float LPS22HB_TEMPERATURE_COUNTS_PER_DEGC = 100.0f;

Libdrivers_Status_t LPS22HB_Init(LPS22HB_Handle_t *pHandle, const LPS22HB_Config_t *pConfig) {

    // The return type from writing to the register
    Libdrivers_Status_t status;

    // Write to CTRL_REG 1->3
    status = LPS22HB_WriteReg(pHandle, LPS22HB_REG_CTRL_REG1, pConfig->CtrlReg1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = LPS22HB_WriteReg(pHandle, LPS22HB_REG_CTRL_REG2, pConfig->CtrlReg2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = LPS22HB_WriteReg(pHandle, LPS22HB_REG_CTRL_REG3, pConfig->CtrlReg3);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t LPS22HB_ReadReg(LPS22HB_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                    uint16_t Length) {

    // Set bit 7 (auto-increment bit) to enable multi-byte reads
    uint8_t ActualAddress = RegAddress | LPS22HB_AUTO_INCREMENT_BIT;

    // Pass the ReadReg arguments to the bus read function
    return pHandle->bus.read(pHandle->bus.ctx, ActualAddress, pBuffer, Length);
}

Libdrivers_Status_t LPS22HB_WriteReg(LPS22HB_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value) {

    // Pass the WriteReg arguments to the bus write function
    return pHandle->bus.write(pHandle->bus.ctx, RegAddress, &Value, 1);
}

Libdrivers_Status_t LPS22HB_ReadRawOutput(LPS22HB_Handle_t *pHandle) {

    // Buffer to store the raw pressure and temperature output values
    uint8_t out[5];

    Libdrivers_Status_t status =
        LPS22HB_ReadReg(pHandle, LPS22HB_REG_PRESS_OUT_XL, out, sizeof(out));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Each field's buffer index is (its register address - 0x28, the burst base).
    // Pressure is 24-bit two's complement; sign-extend the MSB through int8_t before
    // widening so a negative reading doesn't come out as a large positive int32_t.
    pHandle->P_OUT =
        ((int32_t)(int8_t)out[LPS22HB_REG_PRESS_OUT_H - LPS22HB_REG_PRESS_OUT_XL] << 16 |
         (out[LPS22HB_REG_PRESS_OUT_L - LPS22HB_REG_PRESS_OUT_XL] << 8) |
         out[LPS22HB_REG_PRESS_OUT_XL - LPS22HB_REG_PRESS_OUT_XL]);
    pHandle->T_OUT = (out[LPS22HB_REG_TEMP_OUT_H - LPS22HB_REG_PRESS_OUT_XL] << 8) |
                     out[LPS22HB_REG_TEMP_OUT_L - LPS22HB_REG_PRESS_OUT_XL];

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t LPS22HB_CheckWhoAmI(LPS22HB_Handle_t *pHandle) {
    return Libdrivers_Bus_CheckWhoAmI(&pHandle->bus, LPS22HB_REG_WHO_AM_I, LPS22HB_WHO_AM_I_VALUE);
}

Libdrivers_Status_t LPS22HB_ReadPressure(LPS22HB_Handle_t *pHandle, float *pPressure) {

    Libdrivers_Status_t status = LPS22HB_ReadRawOutput(pHandle);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Convert to hPa
    *pPressure = pHandle->P_OUT / LPS22HB_PRESSURE_COUNTS_PER_HPA;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t LPS22HB_ReadTemperature(LPS22HB_Handle_t *pHandle, float *pTemperature) {

    Libdrivers_Status_t status = LPS22HB_ReadRawOutput(pHandle);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Convert to degC
    *pTemperature = pHandle->T_OUT / LPS22HB_TEMPERATURE_COUNTS_PER_DEGC;

    return LIBDRIVERS_OK;
}
