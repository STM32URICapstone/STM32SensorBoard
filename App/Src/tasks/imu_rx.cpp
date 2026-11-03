#include "tasks/imu_rx.h"
#include "BNO080.h"
#include "Quaternion.h"
#include "bno080_calibration.h"
#include "data_defs.h"
#include "main.h"
#include "message_serializer.h"
#include "projdefs.h"
#include "uart_wrapper.h"
#include "task.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

constexpr uint16_t IMU_REPORT_INTERVAL_US = 2500; // rotation vector every 2.5 ms (400 Hz)
constexpr uint32_t IMU_PRINT_PERIOD_MS = 100;     // print attitude at 10 Hz
constexpr int IMU_MAX_PACKETS_PER_POLL = 8;       // bound the drain loop so we always yield
// constexpr uint32_t IMU_TASK_STACK_WORDS = 1024;   // 4 KB: printf with floats is stack-hungry

void setupImuRxTask() {
    
}

void startImuRxTask(void *argument) {
    JetsonSerializer<IMUData> serializer;
    constexpr int serialized_buffer_size = JetsonSerializer<IMUData>::buffer_size;
    static uint8_t serialized[serialized_buffer_size];
    IMUData data;

    if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET) {
        BNO080_Calibration();
    }

    while (BNO080_Initialization() != 0) {
        printf("BNO080 init failed, retrying in 500 ms\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        // HAL_Delay(500);
    }

    BNO080_enableAccelerometer(IMU_REPORT_INTERVAL_US);
    BNO080_enableRotationVector(IMU_REPORT_INTERVAL_US);

    while (true) {
        bool got_data = false;
        for (int i = 0; i < IMU_MAX_PACKETS_PER_POLL && BNO080_dataAvailable() == 1; i++) {
            got_data = true;
        }

        if(got_data) {
            taskENTER_CRITICAL();
            data.acc_x = BNO080_getAccelX();
            data.acc_y = BNO080_getAccelY();
            data.acc_y = BNO080_getAccelY();
            data.quat_i = BNO080_getQuatI();
            data.quat_j = BNO080_getQuatJ();
            data.quat_k = BNO080_getQuatK();
            data.quat_real = BNO080_getQuatReal();
            taskEXIT_CRITICAL();

            serializer.serialize(data, {serialized, serialized_buffer_size});
            uart_wrapper_enqueue_tx(&huart5, serialized, serialized_buffer_size);
        }

        vTaskDelay(pdMS_TO_TICKS(IMU_REPORT_INTERVAL_US / 1000));
    }
}
