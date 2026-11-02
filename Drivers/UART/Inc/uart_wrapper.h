#pragma once

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include <span>

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

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

struct UARTTxWrappedConfig {
    UART_HandleTypeDef *huart;
    SemaphoreHandle_t tx_complete;
    SemaphoreHandle_t tx_mutex;
    MessageBufferHandle_t tx_msgs;

    uint8_t *flat_buffer;
    uint16_t flat_buffer_size;
};

/**
 * @brief Register a UART configuration to receive callbacks
 * Does not start a request. See: uart_wrapper_start_rx
 * @param config
 */
void uart_wrapper_register_rx(UARTWrappedConfig *config);

/**
 * @brief Start a receive request for this configuration
 * Note: config must be registered first. See: uart_wrapper_register_rx
 * @param config
 */
void uart_wrapper_start_rx(UARTWrappedConfig *config);

void uart_wrapper_register_tx(UARTTxWrappedConfig *config);

int uart_wrapper_enqueue_tx(UART_HandleTypeDef *huart, uint8_t *data, uint16_t size);

void uart_wrapper_send_queued_tx(UARTTxWrappedConfig *config);

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);
