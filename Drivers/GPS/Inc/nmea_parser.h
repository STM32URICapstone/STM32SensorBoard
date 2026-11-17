#pragma once

#include "nmea_data_structs.h"
#include <cstdint>
#include <string_view>

/*
references:
    https://gpsd.gitlab.io/gpsd/NMEA.html
    https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_MessageOverview.html
*/

namespace nmea {
    enum class Talker_ID {
        AI, ///< Alarm Indicator
        AP, ///< Auto Pilot
        BD, ///< BeiDou (china)
        CD, ///< Digital Selective Calling (DSC)
        EC, ///< Electronic Chart Display & Information System
        GA, ///< Galileo Positioning System
        CB, ///< BeiDou (china) again
        GI, ///< NavIC, IRNSS (India)
        GL, ///< GLONASS
        GN, ///< Multiple Satellite Systems
        GP, ///< Global Positioning System receiver
        GQ, ///< QZSS regional GPS augmentation system (Japan)
        HC, ///< Heading/Compass
        HE, ///< Gyro, north seeking
        II, ///< Integrated Instrumentation
        IN, ///< Integrated Navigation
        LC, ///< Loran-C receiver (obsolete)
        PQ, ///< QZSS (Quectel Quirk)
        QZ, ///< QZSS regional GPS augmentation system (Japan) again
        SD, ///< Depth Sounder
        ST, ///< Skytraq
        TI, ///< Turn Indicator
        YX, ///< Transducer
        WI, ///< Weather Instrument
        Unknown
    };

    enum class Sentence_ID {
        DTM, ///< Datum reference information
        GBS, ///< GNSS satellite fault detection
        GGA, ///< Time, position, and fix related data
        GLL, ///< Position data: position fix, time of position fix, and status
        GNS, ///< GNS Fix data
        GRS, ///< GRS range residuals
        GSA, ///< GPS DOP and active satellites
        GST, ///< Position error statistics
        GSV, ///< Number of SVs in view, PRN, elevation, azimuth, and SNR
        HDT, ///< Heading from True North
        LLQ, ///< Leica local position and quality
        MSS, ///< MSK receiver signal
        RMC, ///< Position, Velocity, and Time
        ROT, ///< Rate of turn
        VTG, ///< Actual track made good and speed over ground
        ZDA, ///< UTC day, month, and year, and local time zone offset
        Unknown
    };

    enum class Error {
        Ok = 0,
        // Unsupported_Talker,
        Unsupported_Sentence,
        Malformed_Sentence,
        Malformed_Field,
        Bad_Field_Value,
        Field_Overflow,
        Bad_Checksum,
        No_Fix_Other,
    };

    Error parse(std::string_view sentence, Packet *out);
    Error parse(std::string_view sentence, Packet *out, Sentence_Raw *sentence_raw);

    bool next_sentence(std::string_view *remaining, std::string_view *out);
} // namespace nmea
