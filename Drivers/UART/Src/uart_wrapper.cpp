#include "uart_wrapper.h"
#include "stm32f4xx_hal.h"

// The STM32F439 has 8 UART/USART peripherals.
static UARTWrappedConfig *active_configs[8] = {nullptr};

// Translates the hardware register base address to a 0-7 index
static constexpr int get_uart_index(USART_TypeDef *instance) {
    if (instance == USART1) return 0;
    if (instance == USART2) return 1;
    if (instance == USART3) return 2;
    if (instance == UART4) return 3;
    if (instance == UART5) return 4;
    if (instance == USART6) return 5;
    if (instance == UART7) return 6;
    if (instance == UART8) return 7;
    return -1;
}

void uart_wrapper_register_rx(UARTWrappedConfig *config) {
    auto idx = get_uart_index(config->huart->Instance);
    active_configs[idx] = config;
}

void uart_wrapper_start_rx(UARTWrappedConfig *config) {
    auto data_buffer = config->data_buffers[config->buffer_idx];
    HAL_UARTEx_ReceiveToIdle_IT(config->huart, data_buffer, config->buffer_size);
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    int idx = get_uart_index(huart->Instance);
    if (idx < 0 || active_configs[idx] == nullptr) return;

    UARTWrappedConfig *config = active_configs[idx];

    // span because buffer_idx may change before rcvr task awakes and uses data
    std::span<uint8_t> data{config->data_buffers[config->buffer_idx], size};

    auto awoken = pdFALSE;
    xQueueSendFromISR(*(config->rx_queue), &data, &awoken);

    config->buffer_idx ^= 1; // toggle index
    if (config->requeue_after_rx) uart_wrapper_start_rx(config);

    portYIELD_FROM_ISR(awoken);
}
