#include "libdrivers_stm32_spi.h"
#include "libdrivers_stm32_common.h"

static Libdrivers_Status_t STM32_SPI_Read(void *ctx, uint8_t reg, uint8_t *buf, uint16_t len) {

    Libdrivers_STM32_SPI_Context_t *pContext = (Libdrivers_STM32_SPI_Context_t *)ctx;

    // Read transactions set bit 7 of the address byte (R/W = 1)
    uint8_t address = reg | 0x80;

    // Assert chip select (active low)
    HAL_GPIO_WritePin(pContext->cs_port, pContext->cs_pin, GPIO_PIN_RESET);

    // Phase 1: send the address byte; Phase 2: clock in the data
    HAL_StatusTypeDef HalStatus = HAL_SPI_Transmit(pContext->hspi, &address, 1, HAL_MAX_DELAY);
    if (HalStatus == HAL_OK) {
        HalStatus = HAL_SPI_Receive(pContext->hspi, buf, len, HAL_MAX_DELAY);
    }

    // Always deassert CS, even on error, so the bus is never left stuck
    HAL_GPIO_WritePin(pContext->cs_port, pContext->cs_pin, GPIO_PIN_SET);

    return Libdrivers_STM32_CollapseStatus(HalStatus);
}

static Libdrivers_Status_t STM32_SPI_Write(void *ctx, uint8_t reg, const uint8_t *buf,
                                           uint16_t len) {

    Libdrivers_STM32_SPI_Context_t *pContext = (Libdrivers_STM32_SPI_Context_t *)ctx;

    // Write transactions clear bit 7 of the address byte (R/W = 0)
    uint8_t address = reg & 0x7F;

    // Assert chip select (active low)
    HAL_GPIO_WritePin(pContext->cs_port, pContext->cs_pin, GPIO_PIN_RESET);

    // Phase 1: send the address byte; Phase 2: clock out the data.
    // HAL_SPI_Transmit takes a non-const buffer but does not modify it, so the
    // cast away from const is safe.
    HAL_StatusTypeDef HalStatus = HAL_SPI_Transmit(pContext->hspi, &address, 1, HAL_MAX_DELAY);
    if (HalStatus == HAL_OK) {
        HalStatus = HAL_SPI_Transmit(pContext->hspi, (uint8_t *)buf, len, HAL_MAX_DELAY);
    }

    // Always deassert CS, even on error, so the bus is never left stuck
    HAL_GPIO_WritePin(pContext->cs_port, pContext->cs_pin, GPIO_PIN_SET);

    return Libdrivers_STM32_CollapseStatus(HalStatus);
}

static void STM32_SPI_Delay(void *ctx, uint32_t ms) {

    // Intentionally ignoring ctx as it is unused here
    (void)ctx;

    // Pass in the ms delay value to the HAL
    HAL_Delay(ms);
}

void Libdrivers_STM32_SPI_InitBus(Libdrivers_Bus_t *bus, Libdrivers_STM32_SPI_Context_t *context) {
    bus->read = STM32_SPI_Read;
    bus->write = STM32_SPI_Write;
    bus->delay = STM32_SPI_Delay;
    bus->ctx = context;
}
