#include "tasks/imu.h"
#include "BNO080.h"
#include "Quaternion.h"
#include "bno080_calibration.h"
#include "data_defs.h"
#include "main.h"
#include "message_serializer.h"
#include "tasks/imu.h"
#include "uart_wrapper.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

constexpr uint16_t IMU_REPORT_INTERVAL_US = 2500; // rotation vector every 2.5 ms (400 Hz)
constexpr uint32_t IMU_PRINT_PERIOD_MS = 100;     // print attitude at 10 Hz
constexpr int IMU_MAX_PACKETS_PER_POLL = 8;       // bound the drain loop so we always yield
// constexpr uint32_t IMU_TASK_STACK_WORDS = 1024;   // 4 KB: printf with floats is stack-hungry

constexpr int serialized_buffer_size = JetsonSerializer<IMUData>::buffer_size;
static uint8_t serialized[serialized_buffer_size];
static IMUData data;

void setupImuRxTask() {}

void setupImuTxTask() {}

void startImuRxTask(void *argument) {
    if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET) {
        BNO080_Calibration();
    }

    while (BNO080_Initialization() != 0) {
        printf("BNO080 init failed, retrying in 500 ms\r\n");
        vTaskDelay(pdMS_TO_TICKS(1));
        if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET) {
            BNO080_Calibration();
        }
    }

    BNO080_enableAccelerometer(IMU_REPORT_INTERVAL_US);
    BNO080_enableRotationVector(IMU_REPORT_INTERVAL_US);

    auto data_copy = data;

    while (true) {
        bool got_data = false;
        for (int i = 0; i < IMU_MAX_PACKETS_PER_POLL && BNO080_dataAvailable() == 1; i++) {
            got_data = true;
        }

        if (got_data) {
            data_copy.acc_x = BNO080_getAccelX();
            data_copy.acc_y = BNO080_getAccelY();
            data_copy.acc_z = BNO080_getAccelZ();
            data_copy.quat_i = BNO080_getQuatI();
            data_copy.quat_j = BNO080_getQuatJ();
            data_copy.quat_k = BNO080_getQuatK();
            data_copy.quat_real = BNO080_getQuatReal();
            
            taskENTER_CRITICAL();
            data = data_copy;
            taskEXIT_CRITICAL();
        }

        vTaskDelay(pdMS_TO_TICKS(IMU_REPORT_INTERVAL_US/1000));
    }
}

void startImuTxTask(void *argument) {
    JetsonSerializer<IMUData> serializer;

    auto last_print_ms = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
    auto data_copy = data;
    while (true) {
        taskENTER_CRITICAL();
        data_copy = data;
        taskEXIT_CRITICAL();

        serializer.serialize(data_copy, {serialized, serialized_buffer_size});
        uart_wrapper_enqueue_tx(&huart5, serialized, serialized_buffer_size);

        auto now = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
        if (now - last_print_ms >= 100) {
            last_print_ms = now;
            printf(
                "IMU: acc_x=%.6f acc_y=%.6f acc_z=%.6f quat_i=%.6f quat_j=%.6f quat_k=%.6f "
                "quat_real=%.6f\r\n",
                data_copy.acc_x,
                data_copy.acc_y,
                data_copy.acc_z,
                data_copy.quat_i,
                data_copy.quat_j,
                data_copy.quat_k,
                data_copy.quat_real
            );
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
