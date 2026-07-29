#include "libdrivers_stm32_i2c.h"

// Helper to translate HAL_StatusTypeDef returns to Libdrivers_Status_t returns
static Libdrivers_Status_t CollapseStatus(HAL_StatusTypeDef HalStatus) {
    switch (HalStatus) {
    case HAL_OK:
        return LIBDRIVERS_OK;
    case HAL_TIMEOUT:
        return LIBDRIVERS_ERR_TIMEOUT;
    default:
        return LIBDRIVERS_ERR_BUS;
    }
}

static Libdrivers_Status_t STM32_I2C_Read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len) {

    // Cast it back to typed
    Libdrivers_STM32_I2C_Context_t *pContext = (Libdrivers_STM32_I2C_Context_t *)ctx;

    // Call STM32 HAL library Mem_Read passing hi2c and device_addr from the context
    HAL_StatusTypeDef HalStatus = HAL_I2C_Mem_Read(pContext->hi2c, pContext->device_addr, reg,
                                                   I2C_MEMADD_SIZE_8BIT, buf, len, HAL_MAX_DELAY);

    // Translate STM32 HAL status back to libdrivers status
    return CollapseStatus(HalStatus);
}

static Libdrivers_Status_t STM32_I2C_Write(void *ctx, uint8_t reg, const uint8_t *buf,
                                           uint16_t len) {
    // Cast it back to typed
    Libdrivers_STM32_I2C_Context_t *pContext = (Libdrivers_STM32_I2C_Context_t *)ctx;

    // Call STM32 HAL library Mem_Write passing hi2c and device_addr from the context
    HAL_StatusTypeDef HalStatus = HAL_I2C_Mem_Write(pContext->hi2c, pContext->device_addr, reg,
                                                    I2C_MEMADD_SIZE_8BIT, buf, len, HAL_MAX_DELAY);

    // Translate STM32 HAL status back to libdrivers status
    return CollapseStatus(HalStatus);
}

static void STM32_I2C_Delay(void *ctx, uint32_t ms) {

    // Intentionally ignoring ctx as it is unused here
    (void)ctx;

    // Pass in the ms delay value to the HAL
    HAL_Delay(ms);
}

void Libdrivers_STM32_I2C_InitBus(Libdrivers_Bus_t *bus, Libdrivers_STM32_I2C_Context_t *context) {
    bus->read = STM32_I2C_Read;
    bus->write = STM32_I2C_Write;
    bus->delay = STM32_I2C_Delay;
    bus->ctx = context;
}
