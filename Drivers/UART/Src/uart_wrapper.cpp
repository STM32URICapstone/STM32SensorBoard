#include "uart_wrapper.h"
#include "portmacro.h"
#include "stm32f4xx_hal_def.h"
#include <unordered_map>

SemaphoreHandle_t map_mutex;
std::unordered_map<UART_HandleTypeDef *, UARTWrappedConfig> handle_to_conf;

void uart_wrapper_setup() { map_mutex = xSemaphoreCreateMutex(); }

void uart_wrapper_register_rx(
    UART_HandleTypeDef *huart, uint8_t *data_buffer, uint16_t buffer_size, QueueHandle_t *size_queue
) {
    xSemaphoreTake(map_mutex, pdMS_TO_TICKS(10));
    handle_to_conf.insert_or_assign(
        huart, UARTWrappedConfig{huart, size_queue, data_buffer, buffer_size}
    );
    xSemaphoreGive(map_mutex);
    HAL_UARTEx_ReceiveToIdle_IT(huart, data_buffer, buffer_size);
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    auto awoken = pdFALSE;
    xSemaphoreTakeFromISR(map_mutex, &awoken);

    if (!handle_to_conf.contains(huart)) {
        xSemaphoreGiveFromISR(map_mutex, &awoken);
        portYIELD_FROM_ISR(awoken);
        return;
    }

    auto &conf = handle_to_conf.at(huart);
    auto &handle = conf.rx_size;

    // queue acts as a notification and holds the size of the rx data
    xQueueSendFromISR(*handle, &size, &awoken);
    xSemaphoreGiveFromISR(map_mutex, &awoken);
    portYIELD_FROM_ISR(awoken);

    HAL_UARTEx_ReceiveToIdle_IT(huart, conf.data_buffer, conf.buffer_size);
}
