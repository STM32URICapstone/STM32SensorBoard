#include "nmea_sentence_util.h"
#include <charconv>

namespace nmea {
    constexpr float one_over_60 = 1 / 60.0f;

    Error parse_gll(const Sentence_Raw *sentence_raw, GLL *out) {

        if (sentence_raw->num_fields < 6) return Error::Malformed_Sentence;

        auto &raw_lat = sentence_raw->fields[0];
        auto &raw_lat_dir = sentence_raw->fields[1];
        auto &raw_lon = sentence_raw->fields[2];
        auto &raw_lon_dir = sentence_raw->fields[3];
        auto &raw_time = sentence_raw->fields[4]; // TODO: implement time parsing
        auto &raw_valid = sentence_raw->fields[5];

        if (raw_valid != "A") return Error::No_Fix_Other;

        if (raw_lat.size() < 2) return Error::Malformed_Field;
        if (raw_lat_dir.empty()) return Error::Malformed_Field;
        if (raw_lon.size() < 3) return Error::Malformed_Field;
        if (raw_lon_dir.empty()) return Error::Malformed_Field;

        int degrees;
        float minutes;

        // parse latitude from dd mm.mmmmm format
        auto res = std::from_chars(raw_lat.data(), raw_lat.data() + 2, degrees);
        if (res.ec != std::errc{}) return Error::Bad_Field_Value;

        res = std::from_chars(raw_lat.data() + 2, raw_lat.data() + raw_lat.size(), minutes);
        if (res.ec != std::errc{}) return Error::Bad_Field_Value;

        if (raw_lat_dir[0] == 'N') out->latitude = degrees + minutes * one_over_60;
        else if (raw_lat_dir[0] == 'S') out->latitude = -(degrees + minutes * one_over_60);
        else return Error::Bad_Field_Value;

        // parse longitude from ddd mm.mmmmm format
        res = std::from_chars(raw_lon.data(), raw_lon.data() + 3, degrees);
        if (res.ec != std::errc{}) return Error::Bad_Field_Value;

        res = std::from_chars(raw_lon.data() + 3, raw_lon.data() + raw_lon.size(), minutes);
        if (res.ec != std::errc{}) return Error::Bad_Field_Value;

        if (raw_lon_dir[0] == 'E') out->longitude = degrees + minutes * one_over_60;
        else if (raw_lon_dir[0] == 'W') out->longitude = -(degrees + minutes * one_over_60);
        else return Error::Bad_Field_Value;

        // TODO: implement time parsing
        out->time.hours = 0;
        out->time.minutes = 0;
        out->time.seconds = 0.0f;

        return Error::Ok;
    }
}
