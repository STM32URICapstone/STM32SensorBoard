/**
 * BNO080 sensor calibration for FreeRTOS on NUCLEO-F439ZI.
 *
 * Adapted from "BNO080 Sensor Calibration Source Code for M-HIVE STM32 DIYDrone Development
 * Online Course.c" by ChrisP @ M-HIVE (Rev. 1.2), itself based on the SparkFun BNO080 calibration
 * example by Nathan Seidle.
 *
 * Changes from the course version:
 *  - FS-i6 / iBus switch replaced with the NUCLEO blue USER button (B1)
 *  - Buzzer (TIM3) and PC0-PC2 LEDs replaced with the board LEDs LD1 (green) and LD3 (red)
 *  - HAL_Delay() replaced with vTaskDelay() so other tasks keep running
 *  - Drains all pending sensor reports each pass instead of reading one per 100 ms
 *  - Fixed: printed quatI three times instead of i, j, k
 *  - Fixed: the "saved" check now waits for the actual Save DCD response. The original checked
 *    calibrationStatus, which was already 0 from the earlier calibrateAll() reply, so it
 *    reported success even if the save never happened.
 */

#include "bno080_calibration.h"
#include "BNO080.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Packet buffers from BNO080.c, used to inspect the Save DCD response */
extern uint8_t shtpHeader[4];
extern uint8_t shtpData[MAX_PACKET_SIZE];

#define CAL_PRINT_PERIOD_MS   100
#define CAL_SAVE_TIMEOUT_MS   1000
#define CAL_MAX_PACKETS       16

static int button_pressed(void)
{
	return HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET;
}

static const char *accuracy_str(uint8_t accuracy)
{
	switch (accuracy)
	{
		case 0:  return "Unreliable";
		case 1:  return "Low";
		case 2:  return "Medium";
		case 3:  return "High";
		default: return "?";
	}
}

static void blink(GPIO_TypeDef *port, uint16_t pin, int times)
{
	for (int i = 0; i < times; i++)
	{
		HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
		vTaskDelay(pdMS_TO_TICKS(150));
		HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
		vTaskDelay(pdMS_TO_TICKS(150));
	}
}

int BNO080_Calibration(void)
{
	//Resets BNO080 to disable all output
	if (BNO080_Initialization() != 0)
	{
		printf("Calibration aborted: BNO080 not responding\r\n");
		return -1;
	}

	//Enable dynamic calibration for accelerometer, gyroscope, and magnetometer
	//Enable Game Rotation Vector and Magnetic Field output at 50 Hz
	BNO080_calibrateAll();
	BNO080_enableGameRotationVector(20000);
	BNO080_enableMagnetometer(20000);

	printf("Calibrating BNO080. Move the board through all orientations.\r\n");
	printf("Press the blue USER button to finish and save to the sensor's flash.\r\n");
	printf("Output: mag x, y, z (uTesla), mag accuracy, quat i, j, k, real, quat accuracy\r\n\r\n");

	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET); //Red on = calibrating

	//If the button is still held from boot, wait for it to be released first
	while (button_pressed())
		vTaskDelay(pdMS_TO_TICKS(10));
	vTaskDelay(pdMS_TO_TICKS(50)); //debounce

	//Calibration loop: runs until the button is pressed
	while (!button_pressed())
	{
		int got_data = 0;
		for (int i = 0; i < CAL_MAX_PACKETS && BNO080_dataAvailable() == 1; i++)
			got_data = 1;

		if (got_data)
		{
			float x = BNO080_getMagX();
			float y = BNO080_getMagY();
			float z = BNO080_getMagZ();
			uint8_t accuracy = BNO080_getMagAccuracy();

			float quatI = BNO080_getQuatI();
			float quatJ = BNO080_getQuatJ();
			float quatK = BNO080_getQuatK();
			float quatReal = BNO080_getQuatReal();
			uint8_t sensorAccuracy = BNO080_getQuatAccuracy();

			printf("%f,%f,%f,%s\t%f,%f,%f,%f,%s\r\n",
			       x, y, z, accuracy_str(accuracy),
			       quatI, quatJ, quatK, quatReal, accuracy_str(sensorAccuracy));

			//Green LED on when both accuracies are high
			HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin,
			                  (accuracy == 3 && sensorAccuracy == 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		}

		vTaskDelay(pdMS_TO_TICKS(CAL_PRINT_PERIOD_MS));
	}

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

	//Save the current dynamic calibration data (DCD) to the sensor's flash
	BNO080_saveCalibration();

	//Wait for the Save DCD command response (report 0xF1, command 6). R0 == 0 means success.
	int saved = 0;
	TickType_t start = xTaskGetTickCount();
	while (xTaskGetTickCount() - start < pdMS_TO_TICKS(CAL_SAVE_TIMEOUT_MS))
	{
		if (BNO080_dataAvailable() == 1 &&
		    shtpHeader[2] == CHANNEL_CONTROL &&
		    shtpData[0] == SHTP_REPORT_COMMAND_RESPONSE &&
		    shtpData[2] == COMMAND_DCD)
		{
			saved = (shtpData[5] == 0);
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	if (saved)
	{
		printf("\r\nCalibration data successfully stored\r\n");
		blink(LD1_GPIO_Port, LD1_Pin, 3);
	}
	else
	{
		printf("\r\nCalibration data failed to store. Please try again.\r\n");
		blink(LD3_GPIO_Port, LD3_Pin, 3);
	}

	//Calibration should generally be left on: the BNO080 keeps refining it and
	//auto-saves roughly every 5 minutes. The caller re-initialises the sensor afterwards,
	//which turns off the game rotation vector and magnetometer reports.
	return saved ? 0 : 1;
}