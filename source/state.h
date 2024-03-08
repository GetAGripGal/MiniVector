#pragma once

#include "window.h"
#include "frame.h"
#include "display.h"
#include "renderer.h"

/**
 * @brief The state of the application. Mainly used as the user pointer for the window.
 */
typedef struct mv_state
{
    mv_window *window;
    mv_frame *frame;
    mv_display *display;
    mv_renderer *renderer;
} mv_state;