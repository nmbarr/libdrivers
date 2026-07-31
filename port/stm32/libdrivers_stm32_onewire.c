#include "libdrivers_stm32_onewire.h"
#include <stdint.h>

// Enable the DWT cycle counter, the source of this port's microsecond timing.
// It is off at reset, so the trace subsystem must be enabled and the counter started.
static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enables the trace subsystem that owns DWT
    DWT->CYCCNT = 0;                                // Zero the counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            // Start counting CPU cycles
}

// Busy-wait for at least us microseconds by spinning on the DWT cycle counter.
// The unsigned subtraction stays correct even when CYCCNT wraps past 2^32.
static void delay_us(uint32_t us) {
    uint32_t cycles = us * (SystemCoreClock / 1000000U); // Convert us into CPU cycles
    uint32_t start = DWT->CYCCNT;                        // Mark the starting point

    // Loop while CPU counts are less than us
    while ((DWT->CYCCNT - start) < cycles) {
    }
}

// Drive the open-drain line low
static void ow_pull_low(Libdrivers_STM32_OneWire_Context_t *context) {
    HAL_GPIO_WritePin(context->port, context->pin, GPIO_PIN_RESET);
}

// Release the line; the external pull-up floats it high (open-drain, so SET != driving high)
static void ow_release(Libdrivers_STM32_OneWire_Context_t *context) {
    HAL_GPIO_WritePin(context->port, context->pin, GPIO_PIN_SET);
}

// Sample the current line level
static GPIO_PinState ow_sample(Libdrivers_STM32_OneWire_Context_t *context) {
    return HAL_GPIO_ReadPin(context->port, context->pin);
}

// Emit one write time-slot. The ratio of low-time to release-time encodes the bit;
// interrupts are disabled so nothing can stretch the slot and corrupt the timing.
static void write_bit(Libdrivers_STM32_OneWire_Context_t *context, uint8_t bit) {

    __disable_irq();

    if (bit == 1) {
        // Write 1: brief 6us low pulse, then release for the rest of the slot
        ow_pull_low(context);
        delay_us(6);
        ow_release(context);
        delay_us(64);
    } else {
        // Write 0: hold low for most of the slot, then release
        ow_pull_low(context);
        delay_us(60);
        ow_release(context);
        delay_us(10);
    }

    __enable_irq();
}

// Drive one read time-slot and sample the bit the device presents.
// Interrupts are disabled so the sample lands in the valid window.
static uint8_t read_bit(Libdrivers_STM32_OneWire_Context_t *context) {

    GPIO_PinState result;

    __disable_irq();

    // Master starts the slot with a short low pulse, then releases
    ow_pull_low(context);
    delay_us(6);
    ow_release(context);
    delay_us(9);

    // Sample early in the slot, while the device is still holding its bit
    result = ow_sample(context);

    // Idle out the remainder of the >=60us slot before returning
    delay_us(55);

    __enable_irq();

    // A high line means the device sent a 1
    return (result == GPIO_PIN_SET) ? 1 : 0;
}

// Write one byte, LSB first: send bit 0, then shift the next bit down into bit 0
static void write_byte(Libdrivers_STM32_OneWire_Context_t *context, uint8_t byte) {

    for (int i = 0; i < 8; i++) {
        write_bit(context, byte & 0x01);
        byte >>= 1;
    }
}

// Read one byte, LSB first: each new bit enters at bit 7 and rides down toward bit 0,
// so the first bit read ends up as the LSB
static uint8_t read_byte(Libdrivers_STM32_OneWire_Context_t *context) {

    uint8_t byte = 0;

    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (read_bit(context)) {
            byte |= 0x80;
        }
    }

    return byte;
}

// Reset the line and detect a device. Interrupts are disabled so the presence
// pulse is sampled in its valid window.
static Libdrivers_Status_t ow_reset(Libdrivers_STM32_OneWire_Context_t *context) {

    GPIO_PinState result;

    __disable_irq();

    // Long low pulse resets every device on the line, then release
    ow_pull_low(context);
    delay_us(480);
    ow_release(context);
    delay_us(70);

    // A present device answers by pulling the line low, so low here means present
    result = ow_sample(context);

    // Idle out the rest of the presence window before anyone drives the line again
    delay_us(410);

    __enable_irq();

    return (result == GPIO_PIN_RESET) ? LIBDRIVERS_OK : LIBDRIVERS_ERR_BUS;
}

// Contract adapters: cast the opaque ctx back to the typed context, then call
// the layer below. reset already returns a status; write/read can't fail, so
// they run the byte op and report OK.
static Libdrivers_Status_t STM32_OW_Reset(void *ctx) {
    return ow_reset((Libdrivers_STM32_OneWire_Context_t *)ctx);
}

static Libdrivers_Status_t STM32_OW_Write(void *ctx, uint8_t byte) {
    write_byte((Libdrivers_STM32_OneWire_Context_t *)ctx, byte);
    return LIBDRIVERS_OK;
}

static Libdrivers_Status_t STM32_OW_Read(void *ctx, uint8_t *byte) {
    *byte = read_byte((Libdrivers_STM32_OneWire_Context_t *)ctx);
    return LIBDRIVERS_OK;
}

// Turn on the DWT cycle counter this port's timing depends on, then install the
// hooks and context. The caller owns pin configuration (open-drain + pull-up).
void Libdrivers_STM32_OneWire_InitBus(Libdrivers_OneWire_t *ow,
                                      Libdrivers_STM32_OneWire_Context_t *context) {
    DWT_Init();
    ow->reset = STM32_OW_Reset;
    ow->write = STM32_OW_Write;
    ow->read = STM32_OW_Read;
    ow->ctx = context;
}
