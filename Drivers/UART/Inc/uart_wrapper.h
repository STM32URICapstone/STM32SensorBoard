#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "FreeRTOS.h"
#include "semphr.h"

struct UARTWrappedConfig {
    UART_HandleTypeDef *huart;
    QueueHandle_t *rx_size;
    uint8_t* data_buffer;
    uint16_t buffer_size;
};

void uart_wrapper_setup();
void uart_wrapper_register_rx(UART_HandleTypeDef *huart, uint8_t* data_buffer, uint16_t buffer_size, QueueHandle_t *size_queue);

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);
