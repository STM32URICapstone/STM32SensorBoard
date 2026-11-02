#include "appmain.h"
#include "main.h"
#include <cstring>

#include "tasks/debug_print.h"
#include "tasks/gps_rx.h"
#include "uart_wrapper.h"

void AppSetup() {
    setupGpsRxTask();
    xTaskCreate(startGpsRxTask, "gps uart passthrough", 128, NULL, tskIDLE_PRIORITY + 1, NULL);

    setupDebugPrintTask();
    xTaskCreate(startDebugPrintTask, "debug print", 128, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void StartAppMainTask(void *argument) {
    char hello[] = "Hello World!\r\n";
    auto &huart_debug = huart4;
    // HAL_UART_Transmit(&huart_debug, (uint8_t *)hello, strlen(hello), 100);
    while (true) {
        uart_wrapper_enqueue_tx(&huart_debug, reinterpret_cast<uint8_t *>(hello), strlen(hello));
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
