#include "tasks/debug_print.h"
#include "uart_wrapper.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

constexpr size_t buffer_size = 2048;
static uint8_t buffer[buffer_size];
static uint8_t ring_buffer[2 * buffer_size];
static StaticMessageBuffer_t smb;

constexpr auto huart_debug = &huart4;

UARTWrappedTxConfig config;

void setupDebugPrintTask() {
    config = {
        .huart = huart_debug,
        .tx_msgs = xMessageBufferCreateStatic(sizeof(ring_buffer), ring_buffer, &smb),
        .flat_buffer = buffer,
        .flat_buffer_size = buffer_size
    };

    uart_wrapper_register_tx(&config);
}

void startDebugPrintTask(void *argument) {
    while (true) {
        uart_wrapper_send_queued_tx(&config);
    }
}
