#pragma once

#include "nmea_parser.h"

namespace NMEA_Sentences {
    struct GLL {
        float latitude;
        float longitude;

        struct {
            uint8_t hours;
            uint8_t minutes;
            float seconds;
        } time;
    };

    bool parse_gll(NMEA_Sentence_Raw *sentence_raw, GLL *out);
};
