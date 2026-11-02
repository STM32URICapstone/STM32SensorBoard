#include "tasks/gps_rx.h"
#include "uart_wrapper.h"

constexpr auto huart_debug = &huart4;
constexpr auto huart_gps = &huart7;

constexpr size_t buffer_size = 1024;
static uint8_t rx1[buffer_size];
static uint8_t rx2[buffer_size];

static std::span<uint8_t> rx_data;
static auto rx_queue = xQueueCreate(2, sizeof(rx_data));
static UARTWrappedConfig config;

void setupGpsRxTask() {
    config = {
        .huart = huart_gps,
        .rx_queue = &rx_queue,
        .data_buffers = {rx1, rx2},
        .buffer_size = buffer_size,
        .buffer_idx = 0,
        .requeue_after_rx = true,
    };

    uart_wrapper_register_rx(&config);
}

void startGpsRxTask(void *argument) {
    uart_wrapper_start_rx(&config);

    while (true) {
        if (xQueueReceive(rx_queue, &rx_data, portMAX_DELAY) == pdPASS) {
            uart_wrapper_enqueue_tx(huart_debug, rx_data.data(), rx_data.size());
        }
    }
}
