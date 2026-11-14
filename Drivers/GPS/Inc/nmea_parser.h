#pragma once

#include <cstdint>
#include <string_view>

/*
references:
    https://gpsd.gitlab.io/gpsd/NMEA.html
    https://receiverhelp.trimble.com/alloy-gnss/en-us/NMEA-0183messages_MessageOverview.html
*/

// constexpr uint16_t two_char_to_u16(char[2]);
// constexpr uint32_t three_char_to_u32(char[3]);

#define NMEA_MAX_SENTENCE_LENGTH 82
#define NMEA_MAX_SENTENCE_FIELDS 15 // I don't have an exact number

enum class NMEA_Talker_ID {
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
    ERROR
};

enum class NMEA_Sentence_ID {
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
    ERROR
};

struct NMEA_Sentence_Raw {
    NMEA_Talker_ID talker_id;
    NMEA_Sentence_ID sentence_id;
    std::string_view
        fields[NMEA_MAX_SENTENCE_FIELDS]; ///< non-owning views to underlying buffer fields
    size_t num_fields;
};

struct NMEA_GPS_Data {
    float latitude;
    float longitude;
};

/**
 * @brief parse sentence string into fields
 */
bool nmea_parse_sentence(std::string_view sentence, NMEA_Sentence_Raw *sentence_raw);
bool nmea_is_sentence_gps(NMEA_Sentence_Raw *sentence_raw);
bool nmea_gps_from_raw(
    NMEA_Sentence_Raw *sentence_raw, NMEA_GPS_Data *gps_data
);
