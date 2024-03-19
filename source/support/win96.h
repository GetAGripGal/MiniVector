#pragma once

// Only include this file if we are compiling for Windows 96
#ifdef _WIN96

#include <win96/wex.h>
#include <win96/gui.h>

#include "../point.h"
#include "../state.h"

// State management
static mv_state *win96_state;
static mv_electron_point *win96_position_buffer = NULL;

/**
 * @brief Initializes the Windows 96 support library
 *
 * @param state The state to initialize
 */
void mv_support_win96_init(mv_state *state);

/**
 * @brief The emscripten-compliant event loop for Windows 96
 */
void mv_support_win96_event_loop();

#endif // _WIN96