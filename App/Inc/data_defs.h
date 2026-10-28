enum SensorID : uint8_t { GPS = 1, IMU, ULTRA_SONIC };

struct GPSData {
    constexpr static auto sensor_id = static_cast<uint8_t>(SensorID::GPS);

    float latitude;
    float longitude;
    float timestamp;
};

struct IMUData {
    constexpr static auto sensor_id = static_cast<uint8_t>(SensorID::IMU);

    float acc_x, acc_y, acc_z;
    float yaw, pitch, roll;
    float mag_x, mag_y, mag_z;
};
