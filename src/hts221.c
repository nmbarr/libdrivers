#include "libdrivers/hts221.h"
#include <stdint.h>

Libdrivers_Status_t HTS221_Init(HTS221_Handle_t *pHandle, const HTS221_Config_t *pConfig) {

    // The return type from writing to the register
    Libdrivers_Status_t status;

    // Write to CTRL_REG 1->5
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

    // The byte we are about to read
    uint8_t CalibrationByte;

    // The return type of reading the register
    Libdrivers_Status_t status;

    status = HTS221_ReadReg(pHandle, HTS221_REG_H0_RH_X2, &CalibrationByte, 1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }
    pHandle->H0_rH_x2 = CalibrationByte;

    status = HTS221_ReadReg(pHandle, HTS221_REG_H1_RH_X2, &CalibrationByte, 1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }
    pHandle->H1_rH_x2 = CalibrationByte;

    // Need a 16-bit to store the 10-bit combined value
    uint16_t T0DegC;

    // Stores the LSB
    uint8_t T0Lsb;

    // Stores the MSB
    uint8_t T0Msb;

    status = HTS221_ReadReg(pHandle, HTS221_REG_T0_DEGC_X8, &T0Lsb, 1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = HTS221_ReadReg(pHandle, HTS221_REG_T0_T1_MSB, &T0Msb, 1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Extract T0's MSB's from the byte (located at [1:0])
    T0Msb &= HTS221_T0_MSB_MASK;

    // Combine 8-bit LSB with 2-bit MSB
    T0DegC = (T0Msb << 8) | T0Lsb;

    // Update typedef with combined 10-bit value
    pHandle->T0_degC_x8 = T0DegC;

    // Need a 16-bit to store the 10-bit combined value
    uint16_t T1DegC;

    // Stores the LSB
    uint8_t T1Lsb;

    // Stores the MSB
    uint8_t T1Msb;

    status = HTS221_ReadReg(pHandle, HTS221_REG_T1_DEGC_X8, &T1Lsb, 1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    status = HTS221_ReadReg(pHandle, HTS221_REG_T0_T1_MSB, &T1Msb, 1);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    // Extract T1's MSB's from the byte (located at [3:2])
    T1Msb = (T1Msb & HTS221_T1_MSB_MASK) >> HTS221_T1_MSB_SHIFT;

    // Combine 8-bit LSB with 2-bit MSB
    T1DegC = (T1Msb << 8) | T1Lsb;

    // Update typedef with combined 10-bit value
    pHandle->T1_degC_x8 = T1DegC;

    // Buffer to hold the 2 bytes for the _OUT registers
    uint8_t buffer[2];

    int16_t out;

    status = HTS221_ReadReg(pHandle, HTS221_REG_H0_T0_OUT_L, buffer, 2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    out = (buffer[1] << 8) | buffer[0];
    pHandle->H0_T0_OUT = out;

    status = HTS221_ReadReg(pHandle, HTS221_REG_H1_T0_OUT_L, buffer, 2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    out = (buffer[1] << 8) | buffer[0];
    pHandle->H1_T0_OUT = out;

    status = HTS221_ReadReg(pHandle, HTS221_REG_T0_OUT_L, buffer, 2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    out = (buffer[1] << 8) | buffer[0];
    pHandle->T0_OUT = out;

    status = HTS221_ReadReg(pHandle, HTS221_REG_T1_OUT_L, buffer, 2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    out = (buffer[1] << 8) | buffer[0];
    pHandle->T1_OUT = out;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t HTS221_ReadRawOutput(HTS221_Handle_t *pHandle) {

    // The return type of reading the register
    Libdrivers_Status_t status;

    // Buffer to hold the 2 bytes for the _OUT registers
    uint8_t buffer[2];

    int16_t out;

    status = HTS221_ReadReg(pHandle, HTS221_REG_HUMIDITY_OUT_L, buffer, 2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    out = (buffer[1] << 8) | buffer[0];
    pHandle->H_OUT = out;

    status = HTS221_ReadReg(pHandle, HTS221_REG_TEMP_OUT_L, buffer, 2);
    if (status != LIBDRIVERS_OK) {
        return status;
    }

    out = (buffer[1] << 8) | buffer[0];
    pHandle->T_OUT = out;

    return LIBDRIVERS_OK;
}

Libdrivers_Status_t HTS221_CheckWhoAmI(HTS221_Handle_t *pHandle) {

    // Byte to write the data from the WHO_AM_I register to
    uint8_t WhoAmIByte;

    // Read the WHO_AM_I register (0x0F)
    Libdrivers_Status_t status = HTS221_ReadReg(pHandle, HTS221_REG_WHO_AM_I, &WhoAmIByte, 1);
    if (status != LIBDRIVERS_OK) {
        return status; // Propagate the transport error
    }

    // Read succeeded. Check the ID
    if (WhoAmIByte != HTS221_WHO_AM_I_VALUE) {
        return LIBDRIVERS_ERR_ID;
    }

    // Correct chip. Return OK
    return LIBDRIVERS_OK;
}

float HTS221_ReadTemperature(HTS221_Handle_t *pHandle) {
    float temp;
    float T0_degC = pHandle->T0_degC_x8 / 8.0f;
    float T1_degC = pHandle->T1_degC_x8 / 8.0f;

    Libdrivers_Status_t status = HTS221_ReadRawOutput(pHandle);
    if (status != LIBDRIVERS_OK) {
        temp = -999.0f; // sentinel: read failed, ignore this value
    } else {
        temp = T0_degC + (pHandle->T_OUT - pHandle->T0_OUT) * (T1_degC - T0_degC) /
                             (pHandle->T1_OUT - pHandle->T0_OUT);
    }

    return temp;
}

float HTS221_ReadHumidity(HTS221_Handle_t *pHandle) {
    float humidity;
    float H0_rH = pHandle->H0_rH_x2 / 2.0f;
    float H1_rH = pHandle->H1_rH_x2 / 2.0f;

    Libdrivers_Status_t status = HTS221_ReadRawOutput(pHandle);
    if (status != LIBDRIVERS_OK) {
        humidity = -999.0f; // sentinel: read failed, ignore this value
    } else {
        humidity = H0_rH + (pHandle->H_OUT - pHandle->H0_T0_OUT) * (H1_rH - H0_rH) /
                               (pHandle->H1_T0_OUT - pHandle->H0_T0_OUT);
    }

    return humidity;
}
