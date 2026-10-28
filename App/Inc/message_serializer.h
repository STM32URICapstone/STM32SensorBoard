#pragma once
#include <concepts>
#include <cstdint>
#include <cstring>

#include "data_defs.h"
#include "stm32f4xx_hal.h"

constexpr uint16_t MAX_SENSOR_DATA_SIZE = 0x800 - 12;
constexpr uint32_t SENSOR_SYNC_WORD = 0x55AA55AA;

template <class SensorData_t>
concept ValidSensorData = (sizeof(SensorData_t) <= MAX_SENSOR_DATA_SIZE) &&
                          std::is_trivially_copyable_v<SensorData_t> && requires {
                              { SensorData_t::sensor_id } -> std::convertible_to<uint8_t>;
                          };

struct [[gnu::packed]] JetsonMessageHeader {
    uint32_t sync_word = SENSOR_SYNC_WORD;
    uint8_t sensor_id = 0;
    uint8_t sequence_num = 0;
    uint16_t data_length = 0;
    uint32_t crc = 0;
};

// template <class SensorData_t>
//     requires ValidSensorData<SensorData_t>
// struct [[gnu::packed]] JetsonMessage {
//     JetsonMessageHeader header;
//     SensorData_t data;
// };

template <class SensorData_t>
    requires ValidSensorData<SensorData_t>
struct JetsonSerializer {
    uint8_t sequence_number = 0;
    constexpr static uint8_t sensor_id = SensorData_t::sensor_id;
    constexpr static uint16_t header_length = sizeof(JetsonMessageHeader);
    constexpr static uint16_t data_length = sizeof(SensorData_t);
    constexpr static auto buffer_size = header_length + data_length;

    constexpr auto get_buffer_size() { return buffer_size; }

    uint16_t serialize(const SensorData_t &data, std::span<uint8_t> buffer) {
        if (buffer.size() < buffer_size) {
            return 0; // TODO: split across packets ?
        }
        JetsonMessageHeader header{
            SENSOR_SYNC_WORD, sensor_id, sequence_number++, data_length,
            0 // TODO: initialize HAL CRC component
        };

        std::memcpy(buffer.data(), &header, header_length);
        std::memcpy(buffer.data() + header_length, &data, data_length);

        return buffer_size;
    }
};
