#include "appmain.h"
#include "main.h"
#include <cstdio>
#include <cstring>

#include "message_serializer.h"
#include "tasks/debug_print.h"
#include "tasks/gps_rx.h"
#include "tasks/imu.h"
#include "tasks/jetson_tx.h"
#include "uart_wrapper.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
void AppSetup() {  
    setupGpsRxTask();
    xTaskCreate(startGpsRxTask, "gps uart passthrough", 128, NULL, tskIDLE_PRIORITY + 1, NULL);

    setupDebugPrintTask();
    xTaskCreate(startDebugPrintTask, "debug print", 128, NULL, tskIDLE_PRIORITY + 1, NULL);

    setupJetsonTxTask();
    xTaskCreate(
        startJetsonTxTask, "jetson uart transmitting", 128, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    // setupImuRxTask();
    // xTaskCreate(startImuRxTask, "imu sensor rx task", 128, NULL, tskIDLE_PRIORITY + 2, NULL);
    
    // setupImuTxTask();
    // xTaskCreate(startImuTxTask, "imu sensor tx task", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);

    // if (xTaskCreate(imuTask, "imu", IMU_TASK_STACK_WORDS, NULL, tskIDLE_PRIORITY + 2, NULL) !=
    //     pdPASS) {
    //     // Not enough FreeRTOS heap (configTOTAL_HEAP_SIZE) - show it on the red LED
    //     HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    // }
}

void StartAppMainTask(void *argument) {
    char hello[] = "\r\nHello World!\r\n";
    auto &huart_debug = huart4;
    
    uart_wrapper_enqueue_tx(&huart_debug, reinterpret_cast<uint8_t *>(hello), strlen(hello));
    while (true) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
