#pragma once

#include "state.h"

/**
 * @brief Initializes the legacy application state
 *
 * @param state The state of the application
 */
void mv_init_legacy_state(mv_state *state);

/**
 * @brief Destroys the legacy application state
 *
 * @param state The state of the application
 */
void mv_destroy_legacy_state(mv_state *state);

/**
 * @brief The main loop for the legacy renderer
 *
 * @param state The state of the application
 */
void mv_legacy_loop(mv_state *state);
