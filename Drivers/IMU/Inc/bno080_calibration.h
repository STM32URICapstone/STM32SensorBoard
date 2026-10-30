#ifndef _BNO080_CALIBRATION_H
#define _BNO080_CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interactive BNO080/BNO08x calibration, adapted from the M-HIVE course example
 *        to run inside a FreeRTOS task on the NUCLEO-F439ZI.
 *
 * Must be called from a task (uses vTaskDelay). Output goes to printf (UART4).
 *
 *  1. Resets the sensor and enables dynamic calibration for accel, gyro and mag.
 *  2. Streams magnetometer + game rotation vector accuracy at 10 Hz.
 *     Move the board slowly through all orientations (figure-8s, rotate on every axis).
 *     Green LED (LD1) = both accuracies are High.
 *  3. Press the blue USER button (B1) to finish. Calibration is saved to the sensor's flash.
 *     Green LED blinks 3x = saved, red LED (LD3) blinks 3x = save failed.
 *
 * The sensor keeps the saved calibration across power cycles and keeps refining it on its own.
 *
 * @return 0 if calibration was saved, non-zero on failure
 */
int BNO080_Calibration(void);

#ifdef __cplusplus
}
#endif

#endif