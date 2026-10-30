#include "appmain.h"
#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <cstdio>
#include <cstring>

#include "uart_wrapper.h"
#include "BNO080.h"
#include "Quaternion.h"
#include "bno080_calibration.h"

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

const uint16_t test_size = 2048;
uint8_t test_uart[test_size];

void gpsRxTest(void *argument);
void imuTask(void *argument);

/* ------------------------------------------------------------------------- */
/* Debug UART (UART4), shared by printf and the GPS passthrough               */
/* ------------------------------------------------------------------------- */

static SemaphoreHandle_t debug_uart_mutex = nullptr;

/**
 * @brief Blocking write to the debug UART. Serialised with a mutex once the scheduler runs,
 *        so printf from the IMU task and the GPS passthrough don't collide (HAL would
 *        otherwise return HAL_BUSY and silently drop one of them).
 *        Task context only - never call from an ISR.
 */
static void debug_uart_write(const uint8_t *data, uint16_t len) {
    const bool use_mutex =
        debug_uart_mutex != nullptr && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;

    if (use_mutex) xSemaphoreTake(debug_uart_mutex, portMAX_DELAY);
    HAL_UART_Transmit(&huart4, const_cast<uint8_t *>(data), len, HAL_MAX_DELAY);
    if (use_mutex) xSemaphoreGive(debug_uart_mutex);
}

/**
 * @brief Retarget printf/puts to UART4 (replaces the course's _write() that used USART6).
 *        Overrides the weak _write() in CubeMX's syscalls.c.
 *        Note: to print floats (%f) the linker needs "-u _printf_float" (newlib-nano).
 */
extern "C" int _write(int file, char *ptr, int len) {
    (void)file;
    debug_uart_write(reinterpret_cast<const uint8_t *>(ptr), static_cast<uint16_t>(len));
    return len;
}

/* ------------------------------------------------------------------------- */
/* IMU (BNO08x over SPI2)                                                     */
/* ------------------------------------------------------------------------- */

namespace {
constexpr uint16_t IMU_REPORT_INTERVAL_US = 2500; // rotation vector every 2.5 ms (400 Hz)
constexpr uint32_t IMU_PRINT_PERIOD_MS = 100;     // print attitude at 10 Hz
constexpr int IMU_MAX_PACKETS_PER_POLL = 8;       // bound the drain loop so we always yield
constexpr uint32_t IMU_TASK_STACK_WORDS = 1024;   // 4 KB: printf with floats is stack-hungry
} // namespace

/** Latest attitude from the IMU task. Read it from other tasks with imu_get_attitude(). */
struct ImuAttitude {
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float quat[4];          // i, j, k, real
    float accuracy_rad;     // estimated heading accuracy
    uint8_t accuracy;       // 0 = unreliable, 1 = low, 2 = medium, 3 = high
    TickType_t updated_at;  // tick of the last update
    bool valid;             // false until the first sample arrives
};

static ImuAttitude imu_latest = {};

/**
 * @brief Copy the latest attitude. Safe to call from any task.
 *        (Add "struct ImuAttitude" and this prototype to appmain.h if other files need it.)
 * @return true once at least one sample has been received
 */
bool imu_get_attitude(ImuAttitude &out) {
    taskENTER_CRITICAL();
    out = imu_latest;
    taskEXIT_CRITICAL();
    return out.valid;
}

/**
 * @brief IMU task: replaces the while(1) loop from the course's main.c.
 *
 * Hold the blue USER button (B1) while the board boots to run the calibration routine first.
 */
void imuTask(void *argument) {
    (void)argument;

    if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET) {
        BNO080_Calibration();
    }

    // Same as the course's main.c, but retried instead of carrying on with a dead sensor
    while (BNO080_Initialization() != 0) {
        printf("BNO080 init failed, retrying in 1 s\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    BNO080_enableRotationVector(IMU_REPORT_INTERVAL_US);

    TickType_t last_print = xTaskGetTickCount();
    float q[4];

    while (true) {
        // Read every packet the sensor has waiting (INT low = data ready)
        bool got_data = false;
        for (int i = 0; i < IMU_MAX_PACKETS_PER_POLL && BNO080_dataAvailable() == 1; i++) {
            got_data = true;
        }

        if (got_data) {
            q[0] = BNO080_getQuatI();
            q[1] = BNO080_getQuatJ();
            q[2] = BNO080_getQuatK();
            q[3] = BNO080_getQuatReal();
            const float accuracy_rad = BNO080_getQuatRadianAccuracy();
            const uint8_t accuracy = BNO080_getQuatAccuracy();

            Quaternion_Update(q); // updates BNO080_Roll / BNO080_Pitch / BNO080_Yaw (degrees)

            taskENTER_CRITICAL();
            imu_latest.roll_deg = BNO080_Roll;
            imu_latest.pitch_deg = BNO080_Pitch;
            imu_latest.yaw_deg = BNO080_Yaw;
            memcpy(imu_latest.quat, q, sizeof(q));
            imu_latest.accuracy_rad = accuracy_rad;
            imu_latest.accuracy = accuracy;
            imu_latest.updated_at = xTaskGetTickCount();
            imu_latest.valid = true;
            taskEXIT_CRITICAL();
        }

        // Print at a fixed, slow rate - printing every 400 Hz sample would saturate the UART
        const TickType_t now = xTaskGetTickCount();
        if (now - last_print >= pdMS_TO_TICKS(IMU_PRINT_PERIOD_MS)) {
            last_print = now;
            ImuAttitude a;
            if (imu_get_attitude(a)) {
                printf("%.2f\t%.2f\t%.2f\tacc=%u\r\n", a.roll_deg, a.pitch_deg, a.yaw_deg, a.accuracy);
            }
        }

        // Sleep one tick (1 ms) so lower-priority tasks get CPU time; reports arrive every 2.5 ms
        vTaskDelay(1);
    }
}

/* ------------------------------------------------------------------------- */
/* App setup and other tasks                                                  */
/* ------------------------------------------------------------------------- */

void AppSetup() {
    debug_uart_mutex = xSemaphoreCreateMutex();
    uart_wrapper_setup();

    xTaskCreate(gpsRxTest, "gps uart passthrough", 128, NULL, tskIDLE_PRIORITY + 1, NULL);

    if (xTaskCreate(imuTask, "imu", IMU_TASK_STACK_WORDS, NULL, tskIDLE_PRIORITY + 2, NULL) != pdPASS) {
        // Not enough FreeRTOS heap (configTOTAL_HEAP_SIZE) - show it on the red LED
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief Task to receive GPS data and passthrough to debug UART port
 *
 * @param argument unused
 */
void gpsRxTest(void *argument) {
    auto &huart_gps = huart7;
    auto rx_size_q = xQueueCreate(1, sizeof(uint16_t));
    uart_wrapper_register_rx(&huart_gps, test_uart, test_size, &rx_size_q);

    uint16_t rx_size;
    while (true) {
        if (xQueueReceive(rx_size_q, &rx_size, pdMS_TO_TICKS(10)) == pdPASS) {
            debug_uart_write(test_uart, rx_size);
        }
    }
}

void StartAppMainTask(void *argument) {
    const char hello[] = "Hello World!\r\n";
    debug_uart_write(reinterpret_cast<const uint8_t *>(hello), strlen(hello));
    while (true) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void StartDebugPrintTask(void *argument) {}