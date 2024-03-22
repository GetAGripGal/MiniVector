#pragma once

#include <stdint.h>

#include "color.h"

#define DEFAILT_WINDOW_WIDTH 640
#define DEFAILT_WINDOW_HEIGHT 480

#define DEFAULT_RESOLUTION_WIDTH 320
#define DEFAULT_RESOLUTION_HEIGHT 180

#define DEFAULT_INSTRUCTION_PER_FRAME 1024
#define DEFAULT_FRAME_RATE 60

#define DEFAULT_PRIMARY_COLOR mv_create_color(40, 40, 40)
#define DEFAULT_SECONDARY_COLOR mv_create_color(51, 255, 100)

#define DEFAULT_LINE_WIDTH 2
#define DEFAULT_RADIUS 1
#define DEFAULT_DIM_FACTOR 0.05

#ifdef _WIN32
#define DEFAULT_PIPE "\\\\.\\pipe\\mv_pipe"
#else
#define DEFAULT_PIPE "/tmp/mv_pipe"
#endif

/// @brief Configuration for the application
typedef struct mv_config
{
    struct // Window configuration
    {
        uint32_t width;
        uint32_t height;
        int8_t fullscreen;
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
    struct // The gun
    {
        float radius;
        float dim_factor;
    } gun;
    struct // The executor
    {
        uint32_t instruction_per_frame;
        uint32_t frame_rate;
    } executor;

    char *pipe;       // The pipe to read the instructions
    int8_t legacy;    // Use the legacy renderer
    float line_width; // The line width [Legacy only]
} mv_config;

/**
 * @brief Read the configuration from the command line arguments
 */
mv_config mv_read_config(int32_t argc, char *argv[]);