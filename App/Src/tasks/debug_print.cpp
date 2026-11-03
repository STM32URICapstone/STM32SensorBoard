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

static UARTWrappedTxConfig config;

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

// override to automatically feed printf output through the debug port
extern "C" int _write(int file, char *ptr, int len) {
    (void)file;
    constexpr auto debug_uart = &huart4;

    // guard incase printf called before task initialized
    // using [[likely]] compiler hint because task scheduler will be running ~100% of the time
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) [[likely]] {
        uart_wrapper_enqueue_tx(
            debug_uart, reinterpret_cast<uint8_t *>(ptr), static_cast<uint16_t>(len)
        );
    } else [[unlikely]] {
        HAL_UART_Transmit(
            debug_uart, reinterpret_cast<uint8_t *>(ptr), static_cast<uint16_t>(len), 10
        );
    }
    return len;
}
