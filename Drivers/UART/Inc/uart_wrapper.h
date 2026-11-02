#pragma once

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include <span>

/**
 * @file uart_wrapper.h
 */

/**
 * @brief Holds information about a UART receiver
 *
 */
struct UARTWrappedRxConfig {
    UART_HandleTypeDef *huart;    ///< ptr to uart peripheral handle
    QueueHandle_t *rx_queue;      ///< ptr to queue handle used to receive a span of the rx data
    uint8_t *data_buffers[2];     ///< double buffers of buffer_size
    uint16_t buffer_size;         ///< size of each rx buffer
    uint8_t buffer_idx = 0;       ///< index of currently used buffer
    bool requeue_after_rx = true; ///< auto requeue after the previous rx transfer completes
};

/**
 * @brief Holds information about a UART transmitter
 *
 */
struct UARTWrappedTxConfig {
    UART_HandleTypeDef *huart;     ///< ptr to uart peripheral handle
    SemaphoreHandle_t tx_complete; ///< notifier that transmission has finished
    SemaphoreHandle_t tx_mutex;    ///< serialized access to the UART TX peripheral
    MessageBufferHandle_t tx_msgs; ///< ring buffer for holding queued TX data

    uint8_t *flat_buffer;      ///< DMA/HAL -friendly buffer to receive items from ring buffer
    uint16_t flat_buffer_size; ///< self-explanatory
};

/**
 * @brief Register a UART RX configuration to receive callbacks
 * @note Does not start initial request.
 * @see uart_wrapper_start_rx
 * @param config
 */
void uart_wrapper_register_rx(UARTWrappedRxConfig *config);

/**
 * @brief Start a receive request for this configuration
 * @note config must be registered first.
 * @see uart_wrapper_register_rx
 * @param config
 */
void uart_wrapper_start_rx(UARTWrappedRxConfig *config);

/**
 * @brief Register a UART TX configuration to receive callbacks
 * Note: must be called first before uart_wrapper_enqueue_tx or uart_wrapper_send_queued_tx
 * @param config
 */
void uart_wrapper_register_tx(UARTWrappedTxConfig *config);

/**
 * @brief Copies data into ring buffer queue for transfer.
 * @note: config for huart must be registered first.
 * @see uart_wrapper_register_tx
 * @note: this function has "ownership" over the UART TX peripheral. Do not use any
 * HAL_UART_Transmit_* API.
 *
 * @param huart uart to transmit from (must be registered in a config)
 * @param data tx data buffer
 * @param size tx data buffer size
 * @return int number of bytes queued
 */
int uart_wrapper_enqueue_tx(UART_HandleTypeDef *huart, uint8_t *data, uint16_t size);

/**
 * @brief sends queued data over UART using DMA/IT
 * @note blocks/sleeps until complete
 * @note should be used inside dedicated TX task
 *
 * @param config
 */
void uart_wrapper_send_queued_tx(UARTWrappedTxConfig *config);
