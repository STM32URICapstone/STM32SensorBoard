#include <cstring>
#include "main.h"
#include "appmain.h"
#include "projdefs.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "FreeRTOS.h"
#include "task.h"

#include "uart_wrapper.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

const uint16_t test_size = 2048;
uint8_t test_uart[test_size];

void gpsRxTest(void *argument);

void AppSetup() {
    uart_wrapper_setup();
    xTaskCreate(gpsRxTest, "gps uart passthrough", 128, NULL, tskIDLE_PRIORITY+1, NULL);
}

void gpsRxTest(void *argument) {
    auto& huart_debug = huart4;
    auto& huart_gps = huart7;
    auto rx_size_q = xQueueCreate(1, sizeof(uint16_t));
    uart_wrapper_register_rx(&huart_gps, test_uart, test_size, &rx_size_q);

    uint16_t rx_size;
    while(true) {
        if(xQueueReceive(rx_size_q, &rx_size, pdMS_TO_TICKS(10)) == pdPASS) {
            HAL_UART_Transmit(&huart_debug, test_uart, rx_size, 100);
        }
    }
}

void StartAppMainTask(void *argument) {
    char hello[] = "Hello World!\r\n";
    auto& huart_debug = huart4;
    HAL_UART_Transmit(&huart_debug, (uint8_t*)hello, strlen(hello), 100);
    while(true) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }    
}

void StartDebugPrintTask(void *argument) {

}