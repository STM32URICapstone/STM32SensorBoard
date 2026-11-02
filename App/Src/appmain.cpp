#include "appmain.h"
#include "main.h"
#include <cstring>

#include "message_serializer.h"
#include "tasks/debug_print.h"
#include "tasks/gps_rx.h"
#include "uart_wrapper.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

void AppSetup() {
    setupGpsRxTask();
    xTaskCreate(startGpsRxTask, "gps uart passthrough", 128, NULL, tskIDLE_PRIORITY + 1, NULL);

    setupDebugPrintTask();
    xTaskCreate(startDebugPrintTask, "debug print", 128, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void StartAppMainTask(void *argument) {
    char hello[] = "\r\nHello World!\r\n";
    auto &huart_debug = huart4;
    // HAL_UART_Transmit(&huart_debug, (uint8_t *)hello, strlen(hello), 100);
    JetsonSerializer<IMUData> serializer;
    constexpr int serialized_buffer_size = JetsonSerializer<IMUData>::buffer_size;
    static uint8_t serialized[serialized_buffer_size];

    IMUData imudata_test = {
        .acc_x = 0,
        .acc_y = -9.8,
        .acc_z = 0,
        .yaw = 3.14,
        .pitch = 0,
        .roll = 1.57,
        .mag_x = 0,
        .mag_y = 1,
        .mag_z = 2.5
    };

    serializer.serialize(imudata_test, {serialized, serialized_buffer_size});

    while (true) {
        uart_wrapper_enqueue_tx(&huart_debug, reinterpret_cast<uint8_t *>(hello), strlen(hello));
        uart_wrapper_enqueue_tx(&huart_debug, serialized, serialized_buffer_size);
        uart_wrapper_enqueue_tx(&huart_debug, reinterpret_cast<uint8_t *>(hello), strlen(hello));

        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
