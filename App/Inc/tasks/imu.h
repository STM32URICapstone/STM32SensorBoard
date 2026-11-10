#ifndef TASKS_IMU_H
#define TASKS_IMU_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

// BNO055 IMU tasks (implemented in imu.cpp)
//  - rx task: initialises the BNO055 over I2C2 and polls accel / mag / quaternion at 100 Hz
//  - tx task: serialises the latest IMUData and sends it to the Jetson on UART5
void setupImuRxTask(void);
void startImuRxTask(void *argument);

void setupImuTxTask(void);
void startImuTxTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // TASKS_IMU_H