#pragma once

#include "state.h"

/**
 * @brief Initializes the modern application state
 *
 * @param state The state of the application
 */
void mv_init_modern_state(mv_state *state);

/**
 * @brief Destroys the modern application state
 *
 * @param state The state of the application
 */
void mv_destroy_modern_state(mv_state *state);

/**
 * @brief The main loop for the modern renderer
 *
 * @param state The state of the application
 */
void mv_modern_loop(mv_state *state);

/**
 * @brief Delays the modern renderer to cap the frame rate
 *
 * @param state The state of the application
 * @param delta The time since the last frame
 * @param now The current time
 * @param last The last time
 * @param frame_time The time since the last frame
 */
static void delay_framecap(mv_state *state, double *delta, double *now, double *last, double frame_time);

/**
 * @brief Processes the modern renderer instructions
 *
 * @param state The state of the application
 * @param positions The buffer of positions
 * @param num_positions The number of positions
 */
static void process_modern_instruction(mv_state *state, mv_electron_point *positions, uint32_t *num_positions);

/**
 * @brief Update the window title for performance benchmarks
 *
 * @param state The state of the application
 * @param instruction_execution_time The time to execute the instructions
 * @param compute_execution_time The time to compute the positions
 * @param render_execution_time The time to render the positions
 * @param fps The frames per second
 * @param frame_time The time since the last frame
 * @param now The current time
 */
static void update_benchmark(mv_state *state, double instruction_execution_time, double compute_execution_time, double render_execution_time, double *fps, double *frame_time, double now);