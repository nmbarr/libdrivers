#include "libdrivers/hts221.h"
#include <stdint.h>

// Undoes the "_x8" fixed-point encoding on T0/T1_degC_x8
static const float HTS221_DEGC_X8_SCALE = 8.0f;

// Undoes the "_x2" fixed-point encoding on H0/H1_rH_x2
static const float HTS221_RH_X2_SCALE = 2.0f;

Libdrivers_Status_t HTS221_Init(HTS221_Handle_t *pHandle, const HTS221_Config_t *pConfig) {

    // The return type from writing to the register
    Libdrivers_Status_t status;

    // Write to CTRL_REG 1->3
    status = HTS221_WriteReg(pHandle, HTS221_REG_CTRL_REG1, pConfig->CtrlReg1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = HTS221_WriteReg(pHandle, HTS221_REG_CTRL_REG2, pConfig->CtrlReg2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = HTS221_WriteReg(pHandle, HTS221_REG_CTRL_REG3, pConfig->CtrlReg3);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t HTS221_ReadReg(HTS221_Handle_t *pHandle, uint8_t RegAddress, uint8_t *pBuffer,
                                   uint16_t Length) {

    // Set bit 7 (auto-increment bit) to enable multi-byte reads
    uint8_t ActualAddress = RegAddress | HTS221_AUTO_INCREMENT_BIT;

    // Pass the ReadReg arguments to the bus read function
    return pHandle->bus.read(pHandle->bus.ctx, ActualAddress, pBuffer, Length);
}

Libdrivers_Status_t HTS221_WriteReg(HTS221_Handle_t *pHandle, uint8_t RegAddress, uint8_t Value) {

    // Pass the WriteReg arguments to the bus write function
    return pHandle->bus.write(pHandle->bus.ctx, RegAddress, &Value, 1);
}

Libdrivers_Status_t HTS221_ReadCalibration(HTS221_Handle_t *pHandle) {

    // Buffer to store data from all of the calibration registers
    uint8_t cal[16];

    Libdrivers_Status_t status = HTS221_ReadReg(pHandle, HTS221_REG_H0_RH_X2, cal, sizeof(cal));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // T0 and T1's high bits share this one register; read it once, mask it twice.
    uint8_t msb = cal[HTS221_REG_T0_T1_MSB - HTS221_REG_H0_RH_X2];

    // Each field's buffer index is (its register address - 0x30, the burst base).
    pHandle->H0_rH_x2 = cal[HTS221_REG_H0_RH_X2 - HTS221_REG_H0_RH_X2];
    pHandle->H1_rH_x2 = cal[HTS221_REG_H1_RH_X2 - HTS221_REG_H0_RH_X2];
    pHandle->T0_degC_x8 =
        ((msb & HTS221_T0_MSB_MASK) << 8) | cal[HTS221_REG_T0_DEGC_X8 - HTS221_REG_H0_RH_X2];
    pHandle->T1_degC_x8 = (((msb & HTS221_T1_MSB_MASK) >> HTS221_T1_MSB_SHIFT) << 8) |
                          cal[HTS221_REG_T1_DEGC_X8 - HTS221_REG_H0_RH_X2];
    pHandle->H0_T0_OUT = (cal[HTS221_REG_H0_T0_OUT_H - HTS221_REG_H0_RH_X2] << 8) |
                         cal[HTS221_REG_H0_T0_OUT_L - HTS221_REG_H0_RH_X2];
    pHandle->H1_T0_OUT = (cal[HTS221_REG_H1_T0_OUT_H - HTS221_REG_H0_RH_X2] << 8) |
                         cal[HTS221_REG_H1_T0_OUT_L - HTS221_REG_H0_RH_X2];
    pHandle->T0_OUT = (cal[HTS221_REG_T0_OUT_H - HTS221_REG_H0_RH_X2] << 8) |
                      cal[HTS221_REG_T0_OUT_L - HTS221_REG_H0_RH_X2];
    pHandle->T1_OUT = (cal[HTS221_REG_T1_OUT_H - HTS221_REG_H0_RH_X2] << 8) |
                      cal[HTS221_REG_T1_OUT_L - HTS221_REG_H0_RH_X2];

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t HTS221_ReadRawOutput(HTS221_Handle_t *pHandle) {

    // Buffer to store the raw temp and humidity output values
    uint8_t out[4];

    Libdrivers_Status_t status =
        HTS221_ReadReg(pHandle, HTS221_REG_HUMIDITY_OUT_L, out, sizeof(out));
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Each field's buffer index is (its register address - 0x28, the burst base).
    pHandle->H_OUT = (out[HTS221_REG_HUMIDITY_OUT_H - HTS221_REG_HUMIDITY_OUT_L] << 8) |
                     out[HTS221_REG_HUMIDITY_OUT_L - HTS221_REG_HUMIDITY_OUT_L];
    pHandle->T_OUT = (out[HTS221_REG_TEMP_OUT_H - HTS221_REG_HUMIDITY_OUT_L] << 8) |
                     out[HTS221_REG_TEMP_OUT_L - HTS221_REG_HUMIDITY_OUT_L];

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t HTS221_CheckWhoAmI(HTS221_Handle_t *pHandle) {
    return Libdrivers_Bus_CheckWhoAmI(&pHandle->bus, HTS221_REG_WHO_AM_I, HTS221_WHO_AM_I_VALUE);
}

Libdrivers_Status_t HTS221_ReadTemperature(HTS221_Handle_t *pHandle, float *pTemperature) {

    float T0_degC = pHandle->T0_degC_x8 / HTS221_DEGC_X8_SCALE;
    float T1_degC = pHandle->T1_degC_x8 / HTS221_DEGC_X8_SCALE;

    Libdrivers_Status_t status = HTS221_ReadRawOutput(pHandle);
    if (status != LIBDRIVERS_OK) {
        return status; // Leave *pTemperature untouched on failure
    }

    *pTemperature = T0_degC + (pHandle->T_OUT - pHandle->T0_OUT) * (T1_degC - T0_degC) /
                                  (pHandle->T1_OUT - pHandle->T0_OUT);

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t HTS221_ReadHumidity(HTS221_Handle_t *pHandle, float *pHumidity) {

    float H0_rH = pHandle->H0_rH_x2 / HTS221_RH_X2_SCALE;
    float H1_rH = pHandle->H1_rH_x2 / HTS221_RH_X2_SCALE;

    Libdrivers_Status_t status = HTS221_ReadRawOutput(pHandle);
    if (status != LIBDRIVERS_OK) {
        return status; // Leave *pHumidity untouched on failure
    }

    *pHumidity = H0_rH + (pHandle->H_OUT - pHandle->H0_T0_OUT) * (H1_rH - H0_rH) /
                             (pHandle->H1_T0_OUT - pHandle->H0_T0_OUT);

    return LIBDRIVERS_OK;
}
