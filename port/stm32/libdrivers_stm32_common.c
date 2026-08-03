#include "libdrivers_stm32_common.h"

Libdrivers_Status_t Libdrivers_STM32_CollapseStatus(HAL_StatusTypeDef HalStatus) {
    switch (HalStatus) {
    case HAL_OK:
        return LIBDRIVERS_OK;
    case HAL_TIMEOUT:
        return LIBDRIVERS_ERR_TIMEOUT;
    default:
        return LIBDRIVERS_ERR_BUS;
    }
}
