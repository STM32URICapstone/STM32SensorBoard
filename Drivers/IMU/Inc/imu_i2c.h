#ifndef INC_MPU6050_H_
#define INC_MPU6050_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    float accel_g[3];
    float gyro_dps[3];
    float temp_c;
} mpu6050data;

HAL_StatusTypeDef imu_init(void);

HAL_StatusTypeDef imu_read(mpu6050data* out);

#ifdef __cplusplus
}
#endif

#endif