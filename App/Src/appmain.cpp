#include <cstring>
#include "main.h"
#include "appmain.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "FreeRTOS.h"
#include "task.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

void AppSetup() {

}

void StartAppMainTask(void *argument) {
    char hello[] = "Hello World!\r\n";
    auto& huart_debug = huart4; 
    while(true) {
        HAL_UART_Transmit(&huart_debug, (uint8_t*)hello, strlen(hello), 100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }    
}