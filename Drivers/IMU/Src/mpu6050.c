#include "mpu6050.h"
#include "main.h"
#include "stm32f439xx.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdio.h>

#define DEVICE_ADDRESS  (0x68 << 1)

#define FS_GYRO_250     0
#define FS_GYRO_500     8
#define FS_GYRO_1000    16
#define FS_GYRO_2000    24

#define FS_ACC_2G       0
#define FS_ACC_4G       8
#define FS_ACC_8G       16
#define FS_ACC_16G      24

// Editable Values - Choose scale/timeout
#define CHOSEN_GYRO     FS_GYRO_500
#define CHOSEN_ACC      FS_ACC_4G

#define I2C_TIMEOUT     100 // milliseconds
///////////////////

#if CHOSEN_GYRO == FS_GYRO_250
    #define GYRO_LSB_PER_DPS    131.0f // +-250 dps

#elif CHOSEN_GYRO == FS_GYRO_500
    #define GYRO_LSB_PER_DPS    65.5f // +-500 dps

#elif CHOSEN_GYRO == FS_GYRO_1000
    #define GYRO_LSB_PER_DPS    32.8f // +-1000 dps

#elif CHOSEN_GYRO == FS_GYRO_2000
    #define GYRO_LSB_PER_DPS    16.4f // +-2000 dps

#endif

#if CHOSEN_ACC == FS_ACC_2G
    #define ACCEL_LSB_PER_G     16384.0f // +-2g

#elif CHOSEN_ACC == FS_ACC_4G
    #define ACCEL_LSB_PER_G     8192.0f // +-4g

#elif CHOSEN_ACC == FS_ACC_8G
    #define ACCEL_LSB_PER_G     4096.0f // +-8g

#elif CHOSEN_ACC == FS_ACC_16G
    #define ACCEL_LSB_PER_G     2048.0f // +-16g

#endif

#define REG_CONFIG_GYRO 27
#define REG_CONFIG_ACC  28
#define REG_USR_CTRL    107
#define REG_DATA        59
// #define REG_ACC         59
// #define REG_TEMP        65
// #define REG_GYRO        67
// all of the above addresses come from the mpu6050 datasheet

#if !defined(MPU_SCL_Pin) || !defined(MPU_SDA_Pin)
    #error "Unlabelled I2C pins, please label MPU_SCL and MPU_SDA in the .ioc and regenerate"
#endif

#define MPU_SCL_PORT    MPU_SCL_GPIO_Port
#define MPU_SDA_PORT    MPU_SDA_GPIO_Port

#define PWR_DEVICE_RESET    0x80 // PWR_MGMT_1 bit 7

// weak references to I2C handles in the .ioc file
extern I2C_HandleTypeDef hi2c1 __attribute__((weak));
extern I2C_HandleTypeDef hi2c2 __attribute__((weak));
extern I2C_HandleTypeDef hi2c3 __attribute__((weak));

static I2C_HandleTypeDef *mpu_i2c = NULL;
#define MPU_I2C         (*mpu_i2c)

// struct to keep track of values we need to find 
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    I2C_TypeDef  *instance;
} mpu_i2c_pin_t;

// these 2 arrays lists every pin that can carry I2C1, I2C2, or I2C3
static const mpu_i2c_pin_t mpu_scl_pins[] = {
    { GPIOB, GPIO_PIN_6,  I2C1 },
    { GPIOB, GPIO_PIN_8,  I2C1 },
    { GPIOB, GPIO_PIN_10, I2C2 },
    { GPIOF, GPIO_PIN_1,  I2C2 },
    { GPIOH, GPIO_PIN_4,  I2C2 },
    { GPIOA, GPIO_PIN_8,  I2C3 },
    { GPIOH, GPIO_PIN_7,  I2C3 },
};

static const mpu_i2c_pin_t mpu_sda_pins[] = {
    { GPIOB, GPIO_PIN_7,  I2C1 },
    { GPIOB, GPIO_PIN_9,  I2C1 },
    { GPIOB, GPIO_PIN_11, I2C2 },
    { GPIOF, GPIO_PIN_0,  I2C2 },
    { GPIOH, GPIO_PIN_5,  I2C2 },
    { GPIOC, GPIO_PIN_9,  I2C3 },
    { GPIOH, GPIO_PIN_8,  I2C3 },
};

// returns I2C peripheral that a pin belongs to
static I2C_TypeDef *mpu6050_pin_to_i2c(const mpu_i2c_pin_t *table, size_t count, GPIO_TypeDef *port, uint16_t pin){
    for(size_t i = 0; i < count; i++){
        if(table[i].port == port && table[i].pin == pin){
            return table[i].instance;
        }
    }
    return NULL;
}

// this function looks i2c handle defined in the ioc (which is reflected in main) on the labelled pins
static I2C_HandleTypeDef *mpu6050_find_i2c(void){
    I2C_TypeDef *scl_i2c = mpu6050_pin_to_i2c(mpu_scl_pins, sizeof(mpu_scl_pins) / sizeof(mpu_scl_pins[0]), MPU_SCL_GPIO_Port, MPU_SCL_Pin);
    I2C_TypeDef *sda_i2c = mpu6050_pin_to_i2c(mpu_sda_pins, sizeof(mpu_sda_pins) / sizeof(mpu_sda_pins[0]), MPU_SDA_GPIO_Port, MPU_SDA_Pin);

    if(scl_i2c == NULL || sda_i2c == NULL){
        printf("MPU_SCL / MPU_SDA labels are not on I2C SCL / SDA pins, check the .ioc\n");
        return NULL;
    }
    if(scl_i2c != sda_i2c){
        printf("MPU_SCL and MPU_SDA are on different I2C peripherals, check the .ioc\n");
        return NULL;
    }

    I2C_HandleTypeDef *handles[] = { &hi2c1, &hi2c2, &hi2c3 };

    for(size_t i = 0; i < sizeof(handles) / sizeof(handles[0]); i++){
        if(handles[i] != NULL && handles[i]->Instance == scl_i2c){
            return handles[i];
        }
    }

    printf("The I2C on the MPU_SCL / MPU_SDA pins is not enabled in the .ioc\n");
    return NULL;
}

static void mpu6050_bus_recover(void){

    // software reset for the MPUs I2C peripheral
    MPU_I2C.Instance->CR1 |= I2C_CR1_SWRST;
    MPU_I2C.Instance->CR1 &= ~I2C_CR1_SWRST;

    HAL_I2C_DeInit(&MPU_I2C);
 
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
 
    HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_Pin, GPIO_PIN_SET);
    gpio.Pin = MPU_SCL_Pin;
    HAL_GPIO_Init(MPU_SCL_PORT, &gpio);
    gpio.Pin = MPU_SDA_Pin;
    HAL_GPIO_Init(MPU_SDA_PORT, &gpio);
    HAL_Delay(1);
 
    for(int i = 0; i < 9; i++){
        if(HAL_GPIO_ReadPin(MPU_SDA_PORT, MPU_SDA_Pin) == GPIO_PIN_SET){
            break;
        }
        HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_Pin, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_Pin, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_Pin, GPIO_PIN_SET);
    HAL_Delay(1);


    HAL_I2C_Init(&MPU_I2C);
}

HAL_StatusTypeDef mpu6050_init(void){

    mpu_i2c = mpu6050_find_i2c();
    if(mpu_i2c == NULL){
        return HAL_ERROR;
    }

    // hopefully fixes reset error
    mpu6050_bus_recover();

    // HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);
    
    if(HAL_I2C_IsDeviceReady(&MPU_I2C, DEVICE_ADDRESS, 1, I2C_TIMEOUT) == HAL_OK){
        printf("Device is ready\n");
    } else{
        printf("The device is not ready, check cables\n");
        return HAL_ERROR;
    }

    // sleep mode
    uint8_t temp_data_sleep = 0;

    if(HAL_I2C_Mem_Write(&MPU_I2C, DEVICE_ADDRESS, REG_USR_CTRL, 1, &temp_data_sleep, 1, I2C_TIMEOUT) == HAL_OK){
        printf("Exiting sleep mode\n");
    } else{
        printf("Failed to exit sleep mode\n");
        return HAL_ERROR;
    }

    // accelerometer init
    uint8_t temp_data_accel = CHOSEN_ACC;

    if(HAL_I2C_Mem_Write(&MPU_I2C, DEVICE_ADDRESS, REG_CONFIG_ACC, 1, &temp_data_accel, 1, I2C_TIMEOUT) == HAL_OK){
        printf("Configuring accelerometer\n");
    } else{
        printf("Accelerometer is not ready, check cables\n");
        return HAL_ERROR;
    }

    // gyroscope init
    uint8_t temp_data_gyro = CHOSEN_GYRO;

    if(HAL_I2C_Mem_Write(&MPU_I2C, DEVICE_ADDRESS, REG_CONFIG_GYRO, 1, &temp_data_gyro, 1, I2C_TIMEOUT) == HAL_OK){
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

    if(mpu_i2c == NULL){
        return HAL_ERROR; // mpu6050_init() hasn't succeeded yet
    }

    // HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(&MPU_I2C, DEVICE_ADDRESS, REG_DATA, 1, data, 14, I2C_TIMEOUT);

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