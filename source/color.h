#pragma once

#include <stdint.h>

/**
 * @brief Represents a color in RGB format
 */
typedef struct mv_color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} mv_color;

/**
 * @brief Creates a color from the given RGB values
 */
mv_color mv_create_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Creates a color from the given hex value
 */
mv_color mv_create_color_from_hex(uint32_t hex);

/**
 * @brief Creates a color from the given string hex value
 */
mv_color mv_create_color_from_hex_str(const char *hex);