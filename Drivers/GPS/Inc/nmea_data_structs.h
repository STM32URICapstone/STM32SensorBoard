#pragma once
#include <cstdint>
#include <string_view>

namespace nmea {
    enum class Talker_ID;
    enum class Sentence_ID;

    constexpr size_t MAX_SENTENCE_LENGTH = 82;
    constexpr size_t MIN_SENTENCE_LENGTH = 12; // minimum sentence: $TTSSS,*hh<CR><LF>
    constexpr size_t MAX_SENTENCE_FIELDS = 24; // theoretical maximum: (82 - 12) / 2 + 1 = 36

    struct Sentence_Raw {
        Talker_ID talker_id;
        Sentence_ID sentence_id;
        std::string_view fields[MAX_SENTENCE_FIELDS]{}; ///< non-owning views to underlying
                                                        ///< buffer fields
        size_t num_fields = 0;
        uint8_t checksum = 0;
    };

    struct GLL {
        float latitude;
        float longitude;

        struct {
            uint8_t hours;
            uint8_t minutes;
            float seconds;
        } time;
    };

    struct Packet {
        Talker_ID talker_id;
        Sentence_ID sentence_id;
        union Data {
            GLL gll;
        } data{};
    };
}
