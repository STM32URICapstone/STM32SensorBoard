#pragma once

#include "nmea_parser.h"

namespace nmea {
    Error parse_gll(const Sentence_Raw *sentence_raw, GLL *out);
};
