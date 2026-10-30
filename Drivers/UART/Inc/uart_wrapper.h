#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include <span>

/**
 * @brief Holds information about a UART receiver
 *
 */
struct UARTWrappedConfig {
    UART_HandleTypeDef *huart; /// ptr to uart peripheral handle
    QueueHandle_t *rx_queue;   /// ptr to queue handle used to receive a span of the rx data
    uint8_t *data_buffers[2];  /// double buffers of buffer_size
    uint16_t buffer_size;      /// size of each rx buffer
    uint8_t buffer_idx = 0;
    bool requeue_after_rx = true; /// auto requeue after the previous rx transfer completes
};

void uart_wrapper_register_rx(UARTWrappedConfig *config);
void uart_wrapper_start_rx(UARTWrappedConfig *config);

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);
