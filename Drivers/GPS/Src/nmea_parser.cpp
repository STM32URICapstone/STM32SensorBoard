#include "nmea_parser.h"
#include <string_view>

using std::string_view;

constexpr uint16_t two_char_to_u16(const char c[2]) {
    return (static_cast<uint16_t>(c[0]) << 8) | static_cast<uint16_t>(c[1]);
}

constexpr uint32_t three_char_to_u32(const char c[3]) {
    return (static_cast<uint32_t>(c[0]) << 16) | (static_cast<uint16_t>(c[1]) << 8) |
           static_cast<uint16_t>(c[2]);
}

constexpr uint8_t hex_to_dec(const char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static NMEA_Talker_ID nmea_get_talker_id(const char id[2]) {
    switch (two_char_to_u16(id)) {
        case two_char_to_u16("AI"):
            return NMEA_Talker_ID::AI;
        case two_char_to_u16("AP"):
            return NMEA_Talker_ID::AP;
        case two_char_to_u16("BD"):
            return NMEA_Talker_ID::BD;
        case two_char_to_u16("CD"):
            return NMEA_Talker_ID::CD;
        case two_char_to_u16("EC"):
            return NMEA_Talker_ID::EC;
        case two_char_to_u16("GA"):
            return NMEA_Talker_ID::GA;
        case two_char_to_u16("CB"):
            return NMEA_Talker_ID::CB;
        case two_char_to_u16("GI"):
            return NMEA_Talker_ID::GI;
        case two_char_to_u16("GL"):
            return NMEA_Talker_ID::GL;
        case two_char_to_u16("GN"):
            return NMEA_Talker_ID::GN;
        case two_char_to_u16("GP"):
            return NMEA_Talker_ID::GP;
        case two_char_to_u16("GQ"):
            return NMEA_Talker_ID::GQ;
        case two_char_to_u16("HC"):
            return NMEA_Talker_ID::HC;
        case two_char_to_u16("HE"):
            return NMEA_Talker_ID::HE;
        case two_char_to_u16("II"):
            return NMEA_Talker_ID::II;
        case two_char_to_u16("IN"):
            return NMEA_Talker_ID::IN;
        case two_char_to_u16("LC"):
            return NMEA_Talker_ID::LC;
        case two_char_to_u16("PQ"):
            return NMEA_Talker_ID::PQ;
        case two_char_to_u16("QZ"):
            return NMEA_Talker_ID::QZ;
        case two_char_to_u16("SD"):
            return NMEA_Talker_ID::SD;
        case two_char_to_u16("ST"):
            return NMEA_Talker_ID::ST;
        case two_char_to_u16("TI"):
            return NMEA_Talker_ID::TI;
        case two_char_to_u16("YX"):
            return NMEA_Talker_ID::YX;
        case two_char_to_u16("WI"):
            return NMEA_Talker_ID::WI;
        default:
            return NMEA_Talker_ID::ERROR;
    }
}

static NMEA_Sentence_ID nmea_get_sentence_id(const char id[3]) {
    switch (three_char_to_u32(id)) {
        case three_char_to_u32("DTM"):
            return NMEA_Sentence_ID::DTM;
        case three_char_to_u32("GBS"):
            return NMEA_Sentence_ID::GBS;
        case three_char_to_u32("GGA"):
            return NMEA_Sentence_ID::GGA;
        case three_char_to_u32("GLL"):
            return NMEA_Sentence_ID::GLL;
        case three_char_to_u32("GNS"):
            return NMEA_Sentence_ID::GNS;
        case three_char_to_u32("GRS"):
            return NMEA_Sentence_ID::GRS;
        case three_char_to_u32("GSA"):
            return NMEA_Sentence_ID::GSA;
        case three_char_to_u32("GST"):
            return NMEA_Sentence_ID::GST;
        case three_char_to_u32("GSV"):
            return NMEA_Sentence_ID::GSV;
        case three_char_to_u32("HDT"):
            return NMEA_Sentence_ID::HDT;
        case three_char_to_u32("LLQ"):
            return NMEA_Sentence_ID::LLQ;
        case three_char_to_u32("MSS"):
            return NMEA_Sentence_ID::MSS;
        case three_char_to_u32("RMC"):
            return NMEA_Sentence_ID::RMC;
        case three_char_to_u32("ROT"):
            return NMEA_Sentence_ID::ROT;
        case three_char_to_u32("VTG"):
            return NMEA_Sentence_ID::VTG;
        case three_char_to_u32("ZDA"):
            return NMEA_Sentence_ID::ZDA;
        default:
            return NMEA_Sentence_ID::ERROR;
    }
}

uint8_t nmea_parse_sentence_fields(string_view str, NMEA_Sentence_Raw *sentence_raw) {
    uint8_t checksum_calc = 0;
    size_t num_fields = 0;
    size_t last_field_start_idx = 0;

    // fields are comma separated
    for (size_t i = 0; i < str.size(); i++) {
        auto c = str[i];
        checksum_calc ^= c;

        if (c == ',') {
            sentence_raw->fields[num_fields] = {
                str.data() + last_field_start_idx, i - last_field_start_idx
            };
            last_field_start_idx = i + 1;
            num_fields++;
        }
    }

    // last field
    sentence_raw->fields[num_fields] = {
        str.data() + last_field_start_idx, str.size() - last_field_start_idx
    };
    sentence_raw->num_fields = num_fields + 1;

    return checksum_calc;
}

// it is required that sentences have 2 char talker id and 3 char sentence id.
bool nmea_parse_sentence(string_view str, NMEA_Sentence_Raw *sentence_raw) {
    const auto sen_len = str.size();
    // minimum sentence: $TTSSS,*hh<CR><LF>
    constexpr size_t min_sen_len = 12;

    if (sen_len < min_sen_len || sen_len > NMEA_MAX_SENTENCE_LENGTH) return false;

    const auto checksum_begin = sen_len - 5;

    // malformed message
    if (str[0] != '$' || str[checksum_begin] != '*' || str[sen_len - 2] != '\r' ||
        str[sen_len - 1] != '\n')
        return false;

    auto str_buf = str.data();

    auto talker = nmea_get_talker_id(str_buf + 1);
    auto sentence = nmea_get_sentence_id(str_buf + 3);

    if (talker == NMEA_Talker_ID::ERROR || sentence == NMEA_Sentence_ID::ERROR) return false;

    const uint8_t checksum =
        hex_to_dec(str[checksum_begin + 1]) << 4 | hex_to_dec(str[checksum_begin + 2]);

    // xor of everything between $ and *, start with IDs
    uint8_t checksum_calc = 0;
    for (auto i = 1; i <= 5; i++) checksum_calc ^= str[i];

    auto checksum_fields =
        nmea_parse_sentence_fields(str.substr(7, sen_len - min_sen_len), sentence_raw);
    checksum_calc ^= checksum_fields;

    return checksum_calc == checksum;
}

bool nmea_is_sentence_gps(NMEA_Sentence_Raw *sentence_raw) {
    using enum NMEA_Sentence_ID;

    switch (sentence_raw->sentence_id) {
        case GSV:
        case GLL:
        case RMC:
            return true;

        default:
            return false;
    }
}


bool nmea_gps_from_raw(NMEA_Sentence_Raw *sentence_raw, NMEA_GPS_Data *gps_data) {
    using enum NMEA_Sentence_ID;

    auto& fields = sentence_raw->fields;

    switch (sentence_raw->sentence_id) {
        // https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_GLL.html
        case GLL: {
            auto raw_lat = fields[0];
            auto raw_lat_dir = fields[1];
            auto raw_long = fields[2];
            auto raw_long_dir = fields[3];
            auto raw_utc = fields[4];
            auto raw_status = fields[5];
            return true;
        };

        default:
            return false;
    }
}
