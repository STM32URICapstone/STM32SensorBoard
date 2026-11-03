#include "uart_wrapper.h"
#include "portmacro.h"
#include "projdefs.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include <cstdint>

// The STM32F439 has 8 UART/USART peripherals.
static UARTWrappedRxConfig *rx_configs[8] = {nullptr};
static UARTWrappedTxConfig *tx_configs[8] = {nullptr};

// Translates the hardware register base address to a 0-7 index
constexpr int get_uart_index(USART_TypeDef *instance) {
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

void uart_wrapper_register_rx(UARTWrappedRxConfig *config) {
    auto idx = get_uart_index(config->huart->Instance);
    if (idx < 0) return;
    rx_configs[idx] = config;
}

void uart_wrapper_start_rx(UARTWrappedRxConfig *config) {
    auto data_buffer = config->data_buffers[config->buffer_idx];
    HAL_UARTEx_ReceiveToIdle_DMA(config->huart, data_buffer, config->buffer_size);
}

void uart_wrapper_register_tx(UARTWrappedTxConfig *config) {
    auto idx = get_uart_index(config->huart->Instance);
    if (idx < 0 || tx_configs[idx] != nullptr) return;
    config->tx_mutex = xSemaphoreCreateMutex();
    config->tx_complete = xSemaphoreCreateBinary();
    tx_configs[idx] = config;
}

void uart_wrapper_send_queued_tx(UARTWrappedTxConfig *config) {
    int idx = get_uart_index(config->huart->Instance);
    if (idx < 0 || tx_configs[idx] == nullptr) return;

    size_t bytes_to_send = xMessageBufferReceive(
        config->tx_msgs, config->flat_buffer, config->flat_buffer_size, portMAX_DELAY
    );

    if (bytes_to_send > 0) {
        HAL_UART_Transmit_DMA(config->huart, config->flat_buffer, bytes_to_send);
        xSemaphoreTake(config->tx_complete, portMAX_DELAY);
    }
}

int uart_wrapper_enqueue_tx(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t size) {
    int idx = get_uart_index(huart->Instance);
    if (idx < 0 || tx_configs[idx] == nullptr) return -1;
    UARTWrappedTxConfig *config = tx_configs[idx];

    xSemaphoreTake(config->tx_mutex, portMAX_DELAY);
    size_t bytes_queued = xMessageBufferSend(config->tx_msgs, data, size, pdMS_TO_TICKS(1));
    xSemaphoreGive(config->tx_mutex);

    return bytes_queued;
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    int idx = get_uart_index(huart->Instance);
    if (idx < 0 || rx_configs[idx] == nullptr) return;

    UARTWrappedRxConfig *config = rx_configs[idx];

    // span because buffer_idx may change before rcvr task awakes and uses data
    std::span<uint8_t> data{config->data_buffers[config->buffer_idx], size};

    auto awoken = pdFALSE;
    xQueueSendFromISR(*(config->rx_queue), &data, &awoken);

    config->buffer_idx ^= 1; // toggle index
    if (config->requeue_after_rx) uart_wrapper_start_rx(config);

    portYIELD_FROM_ISR(awoken);
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    int idx = get_uart_index(huart->Instance);
    if (idx < 0 || tx_configs[idx] == nullptr) return;

    UARTWrappedTxConfig *config = tx_configs[idx];

    auto awoken = pdFALSE;
    xSemaphoreGiveFromISR(config->tx_complete, &awoken);
    portYIELD_FROM_ISR(awoken);
}
