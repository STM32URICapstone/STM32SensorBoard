#include "tasks/imu.h"

// #include "BNO080.h"
// #include "Quaternion.h"
// #include "bno080_calibration.h"
#include "mpu6050.h"

#include "data_defs.h"
#include "main.h"
#include "message_serializer.h"
#include "stm32f4xx_hal_def.h"
#include "tasks/imu.h"
#include "uart_wrapper.h"
#include <stdio.h>

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
    // if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET) {
    //     BNO080_Calibration();
    // }

    while (mpu6050_init() != HAL_OK) {
        printf("mpu6050 init failed, retrying in 500 ms\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    auto data_copy = data;
    mpu6050data out;

    while (true) {
        
        if(mpu6050_read(&out) == HAL_OK){
            data_copy.acc_x = out.accel_g[0];
            data_copy.acc_y = out.accel_g[1];
            data_copy.acc_z = out.accel_g[2];
            data_copy.yaw_vel = out.gyro_dps[0];
            data_copy.pitch_vel = out.gyro_dps[1];
            data_copy.roll_vel = out.gyro_dps[2];
            data_copy.temp_c = out.temp_c;

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
                "IMU: acc_x=%.6f acc_y=%.6f acc_z=%.6f yaw_vel=%.6f pitch_vel=%.6f roll_vel=%.6f temp_c=%.6f\r\n",
                data_copy.acc_x,
                data_copy.acc_y,
                data_copy.acc_z,
                data_copy.yaw_vel,
                data_copy.pitch_vel,
                data_copy.roll_vel,
                data_copy.temp_c
            );
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
