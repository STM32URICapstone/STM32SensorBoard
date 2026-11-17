#include "tasks/gps_rx.h"
#include "nmea_data_structs.h"
#include "nmea_parser.h"
#include "uart_wrapper.h"
#include <cstdio>
#include <string_view>

extern UART_HandleTypeDef huart4; // debug
extern UART_HandleTypeDef huart5; // jetson
extern UART_HandleTypeDef huart7; // gps
extern UART_HandleTypeDef huart3; // st-link virtual com

constexpr auto huart_debug = &huart4;
constexpr auto huart_gps = &huart7;

constexpr size_t buffer_size = 1024;
static uint8_t rx1[buffer_size];
static uint8_t rx2[buffer_size];

static std::span<uint8_t> rx_data;
static auto rx_queue = xQueueCreate(2, sizeof(rx_data));
static UARTWrappedRxConfig config;

static nmea::Sentence_Raw raw;

void setupGpsRxTask() {
    config = {
        .huart = huart_gps,
        .rx_queue = &rx_queue,
        .data_buffers = {rx1, rx2},
        .buffer_size = buffer_size,
        .buffer_idx = 0,
        .requeue_after_rx = true,
    };

    uart_wrapper_register_rx(&config);
}

void startGpsRxTask(void *argument) {
    uart_wrapper_start_rx(&config);

    while (true) {
        if (xQueueReceive(rx_queue, &rx_data, portMAX_DELAY) == pdPASS) {
            uart_wrapper_enqueue_tx(huart_debug, rx_data.data(), rx_data.size());

            std::string_view sentences{
                reinterpret_cast<const char *>(rx_data.data()), rx_data.size()
            };
            std::string_view line;
            nmea::Packet parsed;
            auto& data = parsed.data;
            while (nmea::next_sentence(&sentences, &line)) {
                auto err = nmea::parse(line, &parsed, &raw);
                if (err != nmea::Error::Ok) continue;

                if (parsed.sentence_id == nmea::Sentence_ID::GLL) {
                    printf("lat: %.5f, lon: %.5f\n", data.gll.latitude, data.gll.longitude);
                }
            }

            // if(!remaining.empty()) buffer partial data somewhere?
            // may not be necessary as full 1-second u-blox burst fits entirely in 1 buffer
            // could be useful if other GPS sensors update faster and task can't keep up
        }
    }
}
