#include "mpu6050.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdio.h>

#define DEVICE_ADDRESS  (0x68 << 1)
#define I2C_TIMEOUT     100 // milliseconds

#define FS_GYRO_250     0
#define FS_GYRO_500     8
#define FS_GYRO_1000    16
#define FS_GYRO_2000    24

#define FS_ACC_2G       0
#define FS_ACC_4G       8
#define FS_ACC_8G       16
#define FS_ACC_16G      24

#define GYRO_LSB_PER_DPS    65.5f // +-500 dps
#define ACCEL_LSB_PER_G     8192.0f // +-4g

#define REG_CONFIG_GYRO 27
#define REG_CONFIG_ACC  28
#define REG_USR_CTRL    107
#define REG_DATA        59
// #define REG_ACC         59
// #define REG_TEMP        65
// #define REG_GYRO        67
// all of the above addresses come from the mpu6050 datasheet

#define MPU_SCL_PORT    GPIOF
#define MPU_SCL_PIN     GPIO_PIN_1
#define MPU_SDA_PORT    GPIOF
#define MPU_SDA_PIN     GPIO_PIN_0

#define PWR_DEVICE_RESET    0x80 // PWR_MGMT_1 bit 7

extern I2C_HandleTypeDef hi2c2;

static void mpu6050_bus_recover(void){
    HAL_I2C_DeInit(&hi2c2);
 
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
 
    HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_PIN, GPIO_PIN_SET);
    gpio.Pin = MPU_SCL_PIN;
    HAL_GPIO_Init(MPU_SCL_PORT, &gpio);
    gpio.Pin = MPU_SDA_PIN;
    HAL_GPIO_Init(MPU_SDA_PORT, &gpio);
    HAL_Delay(1);
 
    for(int i = 0; i < 9; i++){
        if(HAL_GPIO_ReadPin(MPU_SDA_PORT, MPU_SDA_PIN) == GPIO_PIN_SET){
            break;
        }
        HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_PIN, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_PIN, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
 
    __HAL_RCC_I2C2_FORCE_RESET();
    __HAL_RCC_I2C2_RELEASE_RESET();
 
    HAL_I2C_Init(&hi2c2);
}

HAL_StatusTypeDef mpu6050_init(void){

    // hopefully fixes reset error
    mpu6050_bus_recover();

    // HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);
    
    if(HAL_I2C_IsDeviceReady(&hi2c2, DEVICE_ADDRESS, 1, I2C_TIMEOUT) == HAL_OK){
        printf("Device is ready\n");
    } else{
        printf("The device is not ready, check cables\n");
        return HAL_ERROR;
    }

    // sleep mode
    uint8_t temp_data_sleep = 0;

    if(HAL_I2C_Mem_Write(&hi2c2, DEVICE_ADDRESS, REG_USR_CTRL, 1, &temp_data_sleep, 1, I2C_TIMEOUT) == HAL_OK){
        printf("Exiting sleep mode\n");
    } else{
        printf("Failed to exit sleep mode\n");
        return HAL_ERROR;
    }

    // accelerometer init
    uint8_t temp_data_accel = FS_ACC_4G;

    if(HAL_I2C_Mem_Write(&hi2c2, DEVICE_ADDRESS, REG_CONFIG_ACC, 1, &temp_data_accel, 1, I2C_TIMEOUT) == HAL_OK){
        printf("Configuring accelerometer\n");
    } else{
        printf("Accelerometer is not ready, check cables\n");
        return HAL_ERROR;
    }

    // gyroscope init
    uint8_t temp_data_gyro = FS_GYRO_500;

    if(HAL_I2C_Mem_Write(&hi2c2, DEVICE_ADDRESS, REG_CONFIG_GYRO, 1, &temp_data_gyro, 1, I2C_TIMEOUT) == HAL_OK){
        printf("Configuring gyro\n");
    } else{
        printf("Gyro is not ready, check cables\n");
        return HAL_ERROR;
    }

    return HAL_OK;
    
}

HAL_StatusTypeDef mpu6050_read(mpu6050data* out){
    // buffer to store sensor data
    // accel: 3 axes, each axis has 2 parts (6 total)
    // temp: 2 parts
    // gyro: accel: 3 axes, each axis has 2 parts (6 total)
    uint8_t data[14];

    // HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(&hi2c2, DEVICE_ADDRESS, REG_DATA, 1, data, 14, I2C_TIMEOUT);

    if(ret != HAL_OK){
        return ret;
    }

    for(int i = 0; i < 3; i++){

        int16_t raw_accel = (int16_t)((data[i * 2] << 8) | data[i * 2 + 1]);

        int16_t raw_gyro = (int16_t)((data[8 + i * 2] << 8) | data[8 + i * 2 + 1]);

        out->accel_g[i] = raw_accel / ACCEL_LSB_PER_G;
        out->gyro_dps[i] = raw_gyro / GYRO_LSB_PER_DPS;
    }

    int16_t raw_temp = (int16_t)((data[6] << 8) | data[7]);
    out->temp_c = raw_temp / 340.0f + 36.53f;

    return HAL_OK;
}