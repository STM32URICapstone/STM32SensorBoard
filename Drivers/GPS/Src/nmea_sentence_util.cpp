#include "nmea_sentence_util.h"
#include <charconv>

namespace NMEA_Sentences {
    constexpr float one_over_60 = 1 / 60.0f;

    bool parse_gll(NMEA_Sentence_Raw *sentence_raw, GLL *out) {
        
        auto &raw_lat = sentence_raw->fields[0];
        auto &raw_lat_dir = sentence_raw->fields[1];
        auto &raw_lon = sentence_raw->fields[2];
        auto &raw_lon_dir = sentence_raw->fields[3];
        
        if (raw_lat.size() < 2) return false;
        if (raw_lat_dir.empty()) return false;
        if (raw_lon.size() < 3) return false;
        if (raw_lon_dir.empty()) return false;
        
        int degrees;
        float minutes;

        // parse latitude from dd mm.mmmmm format
        auto res = std::from_chars(raw_lat.data(), raw_lat.data() + 2, degrees);
        if (res.ec != std::errc{}) return false;

        res = std::from_chars(raw_lat.data() + 2, raw_lat.data() + raw_lat.size(), minutes);
        if (res.ec != std::errc{}) return false;

        
        if (raw_lat_dir[0] == 'N') out->latitude = degrees + minutes * one_over_60;
        else if (raw_lat_dir[0] == 'S') out->latitude = -(degrees + minutes * one_over_60);
        else return false;

        // parse longitude from ddd mm.mmmmm format
        res = std::from_chars(raw_lon.data(), raw_lon.data() + 3, degrees);
        if (res.ec != std::errc{}) return false;

        res = std::from_chars(raw_lon.data() + 3, raw_lon.data() + raw_lon.size(), minutes);
        if (res.ec != std::errc{}) return false;

        if (raw_lon_dir[0] == 'E') out->longitude = degrees + minutes * one_over_60;
        else if (raw_lon_dir[0] == 'W') out->longitude = -(degrees + minutes * one_over_60);
        else return false;

        return true;
    }
}
