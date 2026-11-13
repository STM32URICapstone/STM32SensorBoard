constexpr uint16_t two_char_to_u16(char[2] c) {
    return (static_cast<uint16_t>(c[0]) << 8) | static_cast<uint16_t>(c[1]);
}

constexpr uint32_t three_char_to_u32(char[3] c) {
    return (static_cast<uint32_t>(c[0]) << 16) | (static_cast<uint16_t>(c[1]) << 8) |
           static_cast<uint16_t>(c[2]);
}
