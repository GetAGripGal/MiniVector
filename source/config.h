#pragma once

#include <stdint.h>

#include "color.h"

#define DEFAILT_WINDOW_WIDTH 800
#define DEFAILT_WINDOW_HEIGHT 600

#define DEFAULT_RESOLUTION_WIDTH 320
#define DEFAULT_RESOLUTION_HEIGHT 180

#define DEFAULT_PRIMARY_COLOR mv_create_color(40, 40, 40)
#define DEFAULT_SECONDARY_COLOR mv_create_color(51, 255, 51)

#define DEFAULT_LINE_WIDTH 2

#define DEFAULT_PIPE "/tmp/mv_pipe"

/// @brief Configuration for the application
typedef struct mv_config
{
    struct // Window configuration
    {
        uint32_t width;
        uint32_t height;
    } window;
    struct // Resolution configuration
    {
        uint32_t width;
        uint32_t height;
    } resolution;
    struct // The palette
    {
        mv_color_t primary;
        mv_color_t secondary;
    } palette;
    float line_width; // The line width
    char *pipe;       // The pipe to read the instructions
} mv_config;

/**
 * @brief Read the configuration from the command line arguments
 */
mv_config mv_read_config(int32_t argc, char *argv[]);