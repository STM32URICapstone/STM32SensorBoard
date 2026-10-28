#pragma once

enum SensorID {
    GPS = 1,
    IMU,
    ULTRA_SONIC
}

struct GPSData {
    constexpr int sensor_id = (int)SensorID::GPS;
    float latitude;
    float longitude;
    float timestamp;
}

[[gnu::packed]] template <class SensorData_t>
struct JetsonMessage {
    const uint32_t sync_word = 0x55AA55AA;
    uint8_t sensor_id = 0;
    uint8_t sequence_num = 0;
    SensorData_t data;
    uint32_t crc = 0;
}
