#include "nmea_parser.h"
#include "nmea_sentence_util.h"
#include <string_view>

namespace nmea {

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

    static Talker_ID get_talker_id(const char id[2]) {
        switch (two_char_to_u16(id)) {
            case two_char_to_u16("AI"):
                return Talker_ID::AI;
            case two_char_to_u16("AP"):
                return Talker_ID::AP;
            case two_char_to_u16("BD"):
                return Talker_ID::BD;
            case two_char_to_u16("CD"):
                return Talker_ID::CD;
            case two_char_to_u16("EC"):
                return Talker_ID::EC;
            case two_char_to_u16("GA"):
                return Talker_ID::GA;
            case two_char_to_u16("CB"):
                return Talker_ID::CB;
            case two_char_to_u16("GI"):
                return Talker_ID::GI;
            case two_char_to_u16("GL"):
                return Talker_ID::GL;
            case two_char_to_u16("GN"):
                return Talker_ID::GN;
            case two_char_to_u16("GP"):
                return Talker_ID::GP;
            case two_char_to_u16("GQ"):
                return Talker_ID::GQ;
            case two_char_to_u16("HC"):
                return Talker_ID::HC;
            case two_char_to_u16("HE"):
                return Talker_ID::HE;
            case two_char_to_u16("II"):
                return Talker_ID::II;
            case two_char_to_u16("IN"):
                return Talker_ID::IN;
            case two_char_to_u16("LC"):
                return Talker_ID::LC;
            case two_char_to_u16("PQ"):
                return Talker_ID::PQ;
            case two_char_to_u16("QZ"):
                return Talker_ID::QZ;
            case two_char_to_u16("SD"):
                return Talker_ID::SD;
            case two_char_to_u16("ST"):
                return Talker_ID::ST;
            case two_char_to_u16("TI"):
                return Talker_ID::TI;
            case two_char_to_u16("YX"):
                return Talker_ID::YX;
            case two_char_to_u16("WI"):
                return Talker_ID::WI;
            default:
                return Talker_ID::Unknown;
        }
    }

    static Sentence_ID get_sentence_id(const char id[3]) {
        switch (three_char_to_u32(id)) {
            case three_char_to_u32("DTM"):
                return Sentence_ID::DTM;
            case three_char_to_u32("GBS"):
                return Sentence_ID::GBS;
            case three_char_to_u32("GGA"):
                return Sentence_ID::GGA;
            case three_char_to_u32("GLL"):
                return Sentence_ID::GLL;
            case three_char_to_u32("GNS"):
                return Sentence_ID::GNS;
            case three_char_to_u32("GRS"):
                return Sentence_ID::GRS;
            case three_char_to_u32("GSA"):
                return Sentence_ID::GSA;
            case three_char_to_u32("GST"):
                return Sentence_ID::GST;
            case three_char_to_u32("GSV"):
                return Sentence_ID::GSV;
            case three_char_to_u32("HDT"):
                return Sentence_ID::HDT;
            case three_char_to_u32("LLQ"):
                return Sentence_ID::LLQ;
            case three_char_to_u32("MSS"):
                return Sentence_ID::MSS;
            case three_char_to_u32("RMC"):
                return Sentence_ID::RMC;
            case three_char_to_u32("ROT"):
                return Sentence_ID::ROT;
            case three_char_to_u32("VTG"):
                return Sentence_ID::VTG;
            case three_char_to_u32("ZDA"):
                return Sentence_ID::ZDA;
            default:
                return Sentence_ID::Unknown;
        }
    }

    Error parse_sentence_fields(string_view str, Sentence_Raw *sentence_raw) {
        uint8_t checksum_calc = 0;
        size_t num_fields = 0;
        size_t last_field_start_idx = 0;

        // fields are comma separated
        for (size_t i = 0; i < str.size(); i++) {
            auto c = str[i];
            checksum_calc ^= c;

            if (c == ',') {
                if (num_fields >= MAX_SENTENCE_FIELDS - 1) return Error::Field_Overflow;

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

        sentence_raw->checksum ^= checksum_calc;

        return Error::Ok;
    }

    // it is required that sentences have 2 char talker id and 3 char sentence id.
    Error parse_sentence(string_view str, Sentence_Raw *sentence_raw) {
        const auto sen_len = str.size();

        if (sen_len < MIN_SENTENCE_LENGTH || sen_len > MAX_SENTENCE_LENGTH)
            return Error::Malformed_Sentence;

        const auto checksum_begin = sen_len - 5;

        // malformed message
        if (str[0] != '$' || str[checksum_begin] != '*' || str[sen_len - 2] != '\r' ||
            str[sen_len - 1] != '\n')
            return Error::Malformed_Sentence;

        auto str_buf = str.data();

        auto talker = get_talker_id(str_buf + 1);
        auto sentence = get_sentence_id(str_buf + 3);

        // unknown talker is a soft error
        // if (talker == Talker_ID::Unknown) return Error::Unsupported_Talker;
        if (sentence == Sentence_ID::Unknown) return Error::Unsupported_Sentence;

        sentence_raw->talker_id = talker;
        sentence_raw->sentence_id = sentence;

        const uint8_t checksum =
            hex_to_dec(str[checksum_begin + 1]) << 4 | hex_to_dec(str[checksum_begin + 2]);

        // xor of everything between $ and *, start with IDs + first comma
        for (auto i = 1; i <= 6; i++) sentence_raw->checksum ^= str[i];

        auto err_fields =
            parse_sentence_fields(str.substr(7, sen_len - MIN_SENTENCE_LENGTH), sentence_raw);
        if (err_fields != Error::Ok) return err_fields;

        if (sentence_raw->checksum != checksum) return Error::Bad_Checksum;

        return Error::Ok;
    }

    Error parse(std::string_view sentence, Packet *out) {
        Sentence_Raw raw;
        return parse(sentence, out, &raw);
    }

    Error parse(std::string_view sentence, Packet *out, Sentence_Raw *sentence_raw) {
        // clear existing data, fields left alone
        sentence_raw->num_fields = 0;
        sentence_raw->checksum = 0;
        sentence_raw->talker_id = Talker_ID::Unknown;
        sentence_raw->sentence_id = Sentence_ID::Unknown;

        Error err;

        err = parse_sentence(sentence, sentence_raw);
        if (err != Error::Ok) return err;

        out->talker_id = sentence_raw->talker_id;
        out->sentence_id = sentence_raw->sentence_id;

        switch (sentence_raw->sentence_id) {
            case Sentence_ID::GLL:
                return parse_gll(sentence_raw, &out->gll);

            default:
                return Error::Unsupported_Sentence;
        }
    }

    bool next_sentence(std::string_view *remaining, std::string_view *out) {
        if (!remaining || !out) return false;

        const auto start = remaining->find('$');
        if (start == std::string_view::npos) {
            *remaining = {};
            return false;
        }

        const auto end = remaining->find('\n', start + 1);
        if (end == std::string_view::npos) {
            // Keep incomplete frame starting at '$' for the next chunk
            remaining->remove_prefix(start);
            return false;
        }

        // If another '$' appeared before '\n', the first frame was truncated—resync
        const auto next_dollar = remaining->find('$', start + 1);
        if (next_dollar != std::string_view::npos && next_dollar < end) {
            remaining->remove_prefix(next_dollar);
            return next_sentence(remaining, out);
        }

        *out = remaining->substr(start, end - start + 1);
        remaining->remove_prefix(end + 1);
        return true;
    }
}
