#pragma once

#include "window.h"
#include "legacy/frame.h"
#include "config.h"
#include "pipe.h"
#include "gun.h"
#include "renderer.h"
#include "legacy/display.h"
#include "legacy/renderer.h"

/**
 * @brief The state of the application. Mainly used as the user pointer for the window.
 */
typedef struct mv_state
{
    mv_window *window;

    // The config
    mv_config config;

    // The pipe for reading instructions
    mv_pipe *pipe;

    // The electron renderer
    mv_electron_gun *electron_gun;
    mv_electron_renderer *electron_renderer;

    // Whether the application is in legacy mode
    int8_t legacy;

    // For the legacy renderer
    mv_legacy_display *display_legacy;
    mv_legacy_renderer *renderer_legacy;
    mv_legacy_frame *frame_legacy;
} mv_state;