#include "appmain.h"
#include "FreeRTOS.h"
#include "main.h"
#include "projdefs.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "task.h"
#include <cstring>

#include "uart_wrapper.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

void gpsRxTest(void *argument);

void AppSetup() {
    xTaskCreate(gpsRxTest, "gps uart passthrough", 128, NULL, tskIDLE_PRIORITY + 1, NULL);
}

/**
 * @brief Task to receive GPS data and passthrough to debug UART port
 *
 * @param argument unused
 */
void gpsRxTest(void *argument) {

    constexpr size_t buffer_size = 1024;
    static uint8_t rx1[buffer_size];
    static uint8_t rx2[buffer_size];

    auto &huart_debug = huart4;
    auto &huart_gps = huart7;

    std::span<uint8_t> rx_data;

    auto rx_queue = xQueueCreate(2, sizeof(rx_data));

    UARTWrappedConfig config = {
        .huart = &huart_gps,
        .rx_queue = &rx_queue,
        .data_buffers = {rx1, rx2},
        .buffer_size = buffer_size,
        .buffer_idx = 0,
        .requeue_after_rx = true,
    };

    uart_wrapper_register_rx(&config);
    uart_wrapper_start_rx(&config);

    while (true) {
        if (xQueueReceive(rx_queue, &rx_data, pdMS_TO_TICKS(10)) == pdPASS) {
            HAL_UART_Transmit(&huart_debug, rx_data.data(), rx_data.size(), 100);
        }
    }
}

void StartAppMainTask(void *argument) {
    char hello[] = "Hello World!\r\n";
    auto &huart_debug = huart4;
    HAL_UART_Transmit(&huart_debug, (uint8_t *)hello, strlen(hello), 100);
    while (true) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void StartDebugPrintTask(void *argument) {}
