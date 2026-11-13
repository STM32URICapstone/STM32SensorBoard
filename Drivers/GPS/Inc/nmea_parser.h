#pragma once

#include <cstdint>

constexpr uint16_t two_char_to_u16(char[2]);
constexpr uint32_t three_char_to_u32(char[3]);

// reference: https://gpsd.gitlab.io/gpsd/NMEA.html

enum NMEA_Talker_ID : uint16_t {

};

enum NMEA_Sentence_ID {

};