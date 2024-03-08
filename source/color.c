#include "color.h"
#include <stdio.h>

/**
 * @brief Creates a color from the given RGB values
 */
mv_color_t mv_create_color(uint8_t r, uint8_t g, uint8_t b)
{
    return (mv_color_t){r, g, b};
}

/**
 * @brief Creates a color from the given hex value
 */
mv_color_t mv_create_color_from_hex(uint32_t hex)
{
    return (mv_color_t){(hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF};
}

/**
 * @brief Creates a color from the given string hex value
 */
mv_color_t mv_create_color_from_hex_str(const char *hex)
{
    uint32_t value;
    sscanf(hex, "%x", &value);
    return mv_create_color_from_hex(value);
}